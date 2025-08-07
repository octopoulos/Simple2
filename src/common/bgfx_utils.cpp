// @version 2025-07-11
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include "bgfx_utils.h"

#include <bx/commandline.h>

void calcTangents(void* _vertices, uint16_t _numVertices, bgfx::VertexLayout _layout, const uint16_t* _indices, uint32_t _numIndices)
{
	struct PosTexcoord
	{
		float m_x;
		float m_y;
		float m_z;
		float m_pad0;
		float m_u;
		float m_v;
		float m_pad1;
		float m_pad2;
	};

	float* tangents = new float[6 * _numVertices];
	bx::memSet(tangents, 0, 6 * _numVertices * sizeof(float));

	PosTexcoord v0;
	PosTexcoord v1;
	PosTexcoord v2;

	for (uint32_t ii = 0, num = _numIndices / 3; ii < num; ++ii)
	{
		const uint16_t* indices = &_indices[ii * 3];
		uint32_t        i0      = indices[0];
		uint32_t        i1      = indices[1];
		uint32_t        i2      = indices[2];

		bgfx::vertexUnpack(&v0.m_x, bgfx::Attrib::Position, _layout, _vertices, i0);
		bgfx::vertexUnpack(&v0.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i0);

		bgfx::vertexUnpack(&v1.m_x, bgfx::Attrib::Position, _layout, _vertices, i1);
		bgfx::vertexUnpack(&v1.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i1);

		bgfx::vertexUnpack(&v2.m_x, bgfx::Attrib::Position, _layout, _vertices, i2);
		bgfx::vertexUnpack(&v2.m_u, bgfx::Attrib::TexCoord0, _layout, _vertices, i2);

		const float bax = v1.m_x - v0.m_x;
		const float bay = v1.m_y - v0.m_y;
		const float baz = v1.m_z - v0.m_z;
		const float bau = v1.m_u - v0.m_u;
		const float bav = v1.m_v - v0.m_v;

		const float cax = v2.m_x - v0.m_x;
		const float cay = v2.m_y - v0.m_y;
		const float caz = v2.m_z - v0.m_z;
		const float cau = v2.m_u - v0.m_u;
		const float cav = v2.m_v - v0.m_v;

		const float det    = (bau * cav - bav * cau);
		const float invDet = 1.0f / det;

		const float tx = (bax * cav - cax * bav) * invDet;
		const float ty = (bay * cav - cay * bav) * invDet;
		const float tz = (baz * cav - caz * bav) * invDet;

		const float bx = (cax * bau - bax * cau) * invDet;
		const float by = (cay * bau - bay * cau) * invDet;
		const float bz = (caz * bau - baz * cau) * invDet;

		for (uint32_t jj = 0; jj < 3; ++jj)
		{
			float* tanu = &tangents[indices[jj] * 6];
			float* tanv = &tanu[3];
			tanu[0] += tx;
			tanu[1] += ty;
			tanu[2] += tz;

			tanv[0] += bx;
			tanv[1] += by;
			tanv[2] += bz;
		}
	}

	for (uint32_t ii = 0; ii < _numVertices; ++ii)
	{
		const bx::Vec3 tanu = bx::load<bx::Vec3>(&tangents[ii * 6]);
		const bx::Vec3 tanv = bx::load<bx::Vec3>(&tangents[ii * 6 + 3]);

		float nxyzw[4];
		bgfx::vertexUnpack(nxyzw, bgfx::Attrib::Normal, _layout, _vertices, ii);

		const bx::Vec3 normal = bx::load<bx::Vec3>(nxyzw);
		const float    ndt    = bx::dot(normal, tanu);
		const bx::Vec3 nxt    = bx::cross(normal, tanu);
		const bx::Vec3 tmp    = bx::sub(tanu, bx::mul(normal, ndt));

		float tangent[4];
		bx::store(tangent, bx::normalize(tmp));
		tangent[3] = bx::dot(nxt, tanv) < 0.0f ? -1.0f : 1.0f;

		bgfx::vertexPack(tangent, true, bgfx::Attrib::Tangent, _layout, _vertices, ii);
	}

	delete[] tangents;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

struct RendererTypeRemap
{
	bx::StringView           name;
	bgfx::RendererType::Enum type;
};

static RendererTypeRemap s_rendererTypeRemap[] = {
	{ "d3d11", bgfx::RendererType::Direct3D11 },
	{ "d3d12", bgfx::RendererType::Direct3D12 },
	{ "gl",    bgfx::RendererType::OpenGL     },
	{ "mtl",   bgfx::RendererType::Metal      },
	{ "noop",  bgfx::RendererType::Noop       },
	{ "vk",    bgfx::RendererType::Vulkan     },
};

bx::StringView getName(bgfx::RendererType::Enum _type)
{
	for (uint32_t ii = 0; ii < BX_COUNTOF(s_rendererTypeRemap); ++ii)
	{
		const RendererTypeRemap& remap = s_rendererTypeRemap[ii];

		if (_type == remap.type)
			return remap.name;
	}

	return "";
}

bgfx::RendererType::Enum getType(const bx::StringView& _name)
{
	for (uint32_t ii = 0; ii < BX_COUNTOF(s_rendererTypeRemap); ++ii)
	{
		const RendererTypeRemap& remap = s_rendererTypeRemap[ii];

		if (0 == bx::strCmpI(_name, remap.name))
			return remap.type;
	}

	return bgfx::RendererType::Count;
}

Args::Args(int _argc, const char* const* _argv)
    : m_type(bgfx::RendererType::Count)
    , m_pciId(BGFX_PCI_ID_NONE)
{
	bx::CommandLine cmdLine(_argc, (const char**)_argv);

	if (cmdLine.hasArg("gl"))
	{
		m_type = bgfx::RendererType::OpenGL;
	}
	else if (cmdLine.hasArg("vk"))
	{
		m_type = bgfx::RendererType::Vulkan;
	}
	else if (cmdLine.hasArg("noop"))
	{
		m_type = bgfx::RendererType::Noop;
	}
	else if (cmdLine.hasArg("d3d11"))
	{
		m_type = bgfx::RendererType::Direct3D11;
	}
	else if (cmdLine.hasArg("d3d12"))
	{
		m_type = bgfx::RendererType::Direct3D12;
	}
	else if (BX_ENABLED(BX_PLATFORM_OSX))
	{
		if (cmdLine.hasArg("mtl"))
			m_type = bgfx::RendererType::Metal;
	}

	if (cmdLine.hasArg("amd"))
		m_pciId = BGFX_PCI_ID_AMD;
	else if (cmdLine.hasArg("nvidia"))
		m_pciId = BGFX_PCI_ID_NVIDIA;
	else if (cmdLine.hasArg("intel"))
		m_pciId = BGFX_PCI_ID_INTEL;
	else if (cmdLine.hasArg("sw"))
		m_pciId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
}
