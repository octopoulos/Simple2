// @version 2025-09-27
/*
 * Copyright 2011-2025 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "stdafx.h"
#include "bgfx_utils.h"

#include <bx/commandline.h>

static UMAP_STR_INT bgfxRenderers = {
	{ ""      , bgfx::RendererType::Count      },
	{ "agc"   , bgfx::RendererType::Agc        },
	{ "d3d11" , bgfx::RendererType::Direct3D11 },
	{ "d3d11" , bgfx::RendererType::Direct3D11 },
	{ "d3d12" , bgfx::RendererType::Direct3D12 },
	{ "gl"    , bgfx::RendererType::OpenGL     },
	{ "gles"  , bgfx::RendererType::OpenGLES   },
	{ "gnm"   , bgfx::RendererType::Gnm        },
	{ "mtl"   , bgfx::RendererType::Metal      },
	{ "nvn"   , bgfx::RendererType::Nvn        },
	{ "noop"  , bgfx::RendererType::Noop       },
	{ "opengl", bgfx::RendererType::OpenGL     },
	{ "vk"    , bgfx::RendererType::Vulkan     },
	{ "vulkan", bgfx::RendererType::Vulkan     },
};

static UMAP_STR_INT bgfxVendors = {
	{ ""      , BGFX_PCI_ID_NONE                },
	{ "amd"   , BGFX_PCI_ID_AMD                 },
	{ "apple" , BGFX_PCI_ID_APPLE               },
	{ "intel" , BGFX_PCI_ID_INTEL               },
	{ "ms"    , BGFX_PCI_ID_MICROSOFT           },
	{ "nvidia", BGFX_PCI_ID_NVIDIA              },
	{ "sw"    , BGFX_PCI_ID_SOFTWARE_RASTERIZER },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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

bgfx::RendererType::Enum RendererId(std::string_view name)
{
	return (bgfx::RendererType::Enum)FindDefault(bgfxRenderers, name, bgfx::RendererType::Count);
}

std::string RendererName(int rendererId)
{
	static UMAP_INT_STR bgfxRendererNames;
	if (bgfxRendererNames.empty())
	{
		for (const auto& [name, id] : bgfxRenderers)
			bgfxRendererNames[id] = name;
	}

	return FindDefault(bgfxRendererNames, rendererId, "?");
}

int VendorId(std::string_view name)
{
	return FindDefault(bgfxVendors, name, BGFX_PCI_ID_NONE);
}
