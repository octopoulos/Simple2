// Mesh.cpp
// @author octopoulos
// @version 2025-08-21

#include "stdafx.h"
#include "objects/Mesh.h"
//
#include "loaders/writer.h"
#include "textures/TextureManager.h"
#include "ui/ui.h"
#include "ui/xsettings.h"

#include <meshoptimizer.h>

constexpr uint64_t defaultState = 0
    //| BGFX_STATE_CULL_CCW
    | BGFX_STATE_DEPTH_TEST_LESS
    | BGFX_STATE_MSAA
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_Z;

namespace bgfx
{
	int32_t read(bx::ReaderI* reader, bgfx::VertexLayout& layout, bx::Error* err);
}

void Mesh::ActivatePhysics(bool activate)
{
	body->enabled = activate;
	body->body->setActivationState(activate ? ACTIVE_TAG : DISABLE_SIMULATION);
	body->body->activate(activate);
}

sMesh Mesh::CloneInstance(std::string_view cloneName)
{
	auto clone = std::make_shared<Mesh>(cloneName);
	{
		clone->geometry = geometry;
		clone->groups   = groups;
		clone->type     = type | ObjectType_Clone | ObjectType_Instance;
		clone->type &= ~ObjectType_Group;
	}
	return clone;
}

void Mesh::CreateShapeBody(PhysicsWorld* physics, int shapeType, float mass, const btVector4& dims)
{
	body = std::make_unique<Body>(physics);
	body->CreateShape(shapeType, this, dims);
	body->CreateBody(mass, GlmToBullet(position), GlmToBullet(quaternion));
	type |= ObjectType_HasBody;
}

void Mesh::Destroy()
{
	// clone => don't destroy data
	if (!(type & ObjectType_Clone))
	{
		bx::AllocatorI* allocator = entry::getAllocator();
		for (const auto& group : groups)
		{
			bgfx::destroy(group.m_vbh);
			if (bgfx::isValid(group.m_ibh)) bgfx::destroy(group.m_ibh);

			if (group.m_vertices) bx::free(allocator, group.m_vertices);
			if (group.m_indices) bx::free(allocator, group.m_indices);
		}
		groups.clear();

		// TODO: check if something must be done with the texture?
		// bgfx::destroy(sTexColor);
	}
}

void Mesh::Initialize()
{
	sTexColor  = bgfx::createUniform("s_texColor", bgfx::UniformType::Sampler);
	sTexNormal = bgfx::createUniform("s_texNormal", bgfx::UniformType::Sampler);
}

void Mesh::Load(bx::ReaderSeekerI* reader, bool ramcopy)
{
	constexpr uint32_t kChunkVertexBuffer           = BX_MAKEFOURCC('V', 'B', ' ', 0x1);
	constexpr uint32_t kChunkVertexBufferCompressed = BX_MAKEFOURCC('V', 'B', 'C', 0x0);
	constexpr uint32_t kChunkIndexBuffer            = BX_MAKEFOURCC('I', 'B', ' ', 0x0);
	constexpr uint32_t kChunkIndexBufferCompressed  = BX_MAKEFOURCC('I', 'B', 'C', 0x1);
	constexpr uint32_t kChunkPrimitive              = BX_MAKEFOURCC('P', 'R', 'I', 0x0);

	bx::AllocatorI* allocator = entry::getAllocator();

	uint32_t  chunk;
	bx::Error err;
	Group     group;
	while (bx::read(reader, chunk, &err) == 4 && err.isOk())
	{
		switch (chunk)
		{
		case kChunkVertexBuffer:
		{
			// clang-format off
			read(reader, group.m_sphere, &err);
			read(reader, group.m_aabb  , &err);
			read(reader, group.m_obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			read(reader, group.m_numVertices, &err);
			const bgfx::Memory* mem = bgfx::alloc(group.m_numVertices * stride);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.m_vertices = (uint8_t*)bx::alloc(allocator, group.m_numVertices * stride);
				bx::memCopy(group.m_vertices, mem->data, mem->size);
			}

			group.m_vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkVertexBufferCompressed:
		{
			// clang-format off
			read(reader, group.m_sphere, &err);
			read(reader, group.m_aabb  , &err);
			read(reader, group.m_obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			read(reader, group.m_numVertices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numVertices * stride);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedVertices = bx::alloc(allocator, compressedSize);
			bx::read(reader, compressedVertices, compressedSize, &err);

			meshopt_decodeVertexBuffer(mem->data, group.m_numVertices, stride, (uint8_t*)compressedVertices, compressedSize);

			bx::free(allocator, compressedVertices);

			if (ramcopy)
			{
				group.m_vertices = (uint8_t*)bx::alloc(allocator, group.m_numVertices * stride);
				bx::memCopy(group.m_vertices, mem->data, mem->size);
			}
			group.m_vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkIndexBuffer:
		{
			read(reader, group.m_numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numIndices * 2);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.m_indices = (uint16_t*)bx::alloc(allocator, group.m_numIndices * 2);
				bx::memCopy(group.m_indices, mem->data, mem->size);
			}
			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkIndexBufferCompressed:
		{
			bx::read(reader, group.m_numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.m_numIndices * 2);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedIndices = bx::alloc(allocator, compressedSize);

			bx::read(reader, compressedIndices, compressedSize, &err);

			meshopt_decodeIndexBuffer(mem->data, group.m_numIndices, 2, (uint8_t*)compressedIndices, compressedSize);

			bx::free(allocator, compressedIndices);

			if (ramcopy)
			{
				group.m_indices = (uint16_t*)bx::alloc(allocator, group.m_numIndices * 2);
				bx::memCopy(group.m_indices, mem->data, mem->size);
			}
			group.m_ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkPrimitive:
		{
			uint16_t len;
			read(reader, len, &err);

			std::string material;
			material.resize(len);
			read(reader, const_cast<char*>(material.c_str()), len, &err);

			uint16_t num;
			read(reader, num, &err);

			for (uint32_t ii = 0; ii < num; ++ii)
			{
				read(reader, len, &err);

				std::string name;
				name.resize(len);
				read(reader, const_cast<char*>(name.c_str()), len, &err);

				Primitive prim;
				// clang-format off
				read(reader, prim.m_startIndex , &err);
				read(reader, prim.m_numIndices , &err);
				read(reader, prim.m_startVertex, &err);
				read(reader, prim.m_numVertices, &err);
				read(reader, prim.m_sphere     , &err);
				read(reader, prim.m_aabb       , &err);
				read(reader, prim.m_obb        , &err);
				// clang-format on

				group.m_prims.push_back(prim);
			}

			groups.push_back(group);
			group.Reset();
		}
		break;

		default:
			ui::LogWarning("{:08x} at {}", chunk, bx::skip(reader, 0));
			break;
		}
	}
}

void Mesh::LoadTextures(std::string_view colorName, std::string_view normalName)
{
	if (colorName.size())
	{
		texColor    = GetTextureManager().LoadTexture(colorName);
		texNames[0] = colorName;
	}
	if (normalName.size())
	{
		texNormal   = GetTextureManager().LoadTexture(normalName);
		texNames[1] = normalName;
	}
	Initialize();
}

void Mesh::Render(uint8_t viewId, int renderFlags)
{
	if (!visible) return;

	//if (type & ObjectType_Instance) return;
	//ui::Log("Render: {} {}", type, name);

	const uint64_t useState = state ? state : defaultState;

	if (type & ObjectType_Group)
	{
		if (const uint32_t numChild = TO_UINT32(children.size()))
		{
			if ((type & ObjectType_Instance) && renderFlags & RenderFlag_Instancing)
			{
				// 64 bytes for 4x4 matrix
				const uint16_t stride = 64 + 16;

				// figure out how big of a buffer is available
				const uint32_t drawnChild = bgfx::getAvailInstanceDataBuffer(numChild, stride);
				const auto     missing    = numChild - drawnChild;
				//ui::Log("numChild={} missing={} layout={}", numChild, missing, layout.getStride());

				bgfx::InstanceDataBuffer idb;
				bgfx::allocInstanceDataBuffer(&idb, drawnChild, stride);

				// collect instance data
				if (uint8_t* data = idb.data)
				{
					for (const auto& child : children)
					{
						float* mtx = (float*)data;
						std::memcpy(mtx, glm::value_ptr(child->matrixWorld), 64);

						float* color = (float*)&data[64];
						{
							color[0] = 1.0f;
							color[1] = 1.0f;
							color[2] = 1.0f;
							color[3] = 1.0f;
						}

						data += stride;
					}
				}
				else ui::LogError("allocInstanceDataBuffer");

				// render
				if (material)
				{
					if (geometry)
					{
						bgfx::setVertexBuffer(0, geometry->vbh);
						bgfx::setIndexBuffer(geometry->ibh);
						bgfx::setInstanceDataBuffer(&idb);

						if (bgfx::isValid(texColor)) bgfx::setTexture(0, sTexColor, texColor);
						if (bgfx::isValid(texNormal)) bgfx::setTexture(1, sTexNormal, texNormal);

						bgfx::setState(useState);
						bgfx::submit(viewId, material->program);
					}
					else if (bgfx::isValid(material->program))
					{
						for (const auto& group : groups)
						{
							bgfx::setIndexBuffer(group.m_ibh);
							bgfx::setVertexBuffer(0, group.m_vbh);
							bgfx::setInstanceDataBuffer(&idb);

							if (bgfx::isValid(texColor)) bgfx::setTexture(0, sTexColor, texColor);
							if (bgfx::isValid(texNormal)) bgfx::setTexture(1, sTexNormal, texNormal);

							bgfx::setState(useState);
							bgfx::submit(0, material->program);
							break;
						}
					}
				}
			}
			else
			{
				for (const auto& child : children)
					child->Render(viewId, renderFlags);
			}
		}
	}
	else if (material)
	{
		if (geometry)
		{
			bgfx::setTransform(glm::value_ptr(matrixWorld));
			bgfx::setVertexBuffer(0, geometry->vbh);
			bgfx::setIndexBuffer(geometry->ibh);
			material->Apply();

			if (bgfx::isValid(texColor)) bgfx::setTexture(0, sTexColor, texColor);
			if (bgfx::isValid(texNormal)) bgfx::setTexture(1, sTexNormal, texNormal);

			bgfx::setState(useState);
			bgfx::submit(viewId, material->program);
		}
		else if (bgfx::isValid(material->program))
		{
			Submit(viewId, material->program, glm::value_ptr(matrixWorld), useState);
		}
	}
}

int Mesh::Serialize(fmt::memory_buffer& outString, int bounds) const
{
	// skip some objects
	if (type & (ObjectType_Cursor)) return -1;

	int keyId = Object3d::Serialize(outString, (bounds & 1) ? 1 : 0);
	if (body)
	{
		WRITE_KEY("body");
		body->Serialize(outString);
	}
	if (geometry)
	{
		WRITE_KEY("geometry");
		geometry->Serialize(outString);
	}
	if (load) WRITE_KEY_INT(load);
	if (material)
	{
		WRITE_KEY("material");
		material->Serialize(outString);
	}
	if (modelName.size()) WRITE_KEY_STRING(modelName);
	if (state) WRITE_KEY_INT(state);
	// textures
	{
		int texNum = 4;
		while (texNum > 0 && texNames[texNum - 1].empty()) --texNum;
		if (texNum > 0)
		{
			WRITE_KEY("texNames");
			WRITE_CHAR('[');
			for (int i = 0; i < texNum; ++i)
			{
				if (i > 0) WRITE_CHAR(',');
				WRITE_JSON_STRING(texNames[i]);
			}
			WRITE_CHAR(']');
		}
	}
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Mesh::SetBodyTransform()
{
	position = matrixWorld[3];

	for(int i = 0; i < 3; i++)
		scale[i] = glm::length(glm::vec3(matrixWorld[i]));

	const glm::mat3 rotMtx(
		glm::vec3(matrixWorld[0]) / scale[0],
		glm::vec3(matrixWorld[1]) / scale[1],
		glm::vec3(matrixWorld[2]) / scale[2]);

	quaternion  = glm::quat_cast(rotMtx);
	rotation    = glm::eulerAngles(quaternion);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
	UpdateLocalMatrix("SetBodyTransform");

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	transform.setRotation(btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w));

	// update transform
	body->body->setWorldTransform(transform);
	if (auto* motionState = body->body->getMotionState())
		motionState->setWorldTransform(transform);
}

void Mesh::ShowTable()
{
	Object3d::ShowTable();
	if (body) body->ShowTable();

	// clang-format off
	ui::ShowTable({
		{ "state", std::to_string(state) },
	});
	// clang-format on
}

void Mesh::Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const
{
	// BGFX_STATE_CULL_CCW
	if (state == BGFX_STATE_MASK) state = defaultState;

	bgfx::setTransform(mtx);
	bgfx::setState(state);

	for (const auto& group : groups)
	{
		bgfx::setIndexBuffer(group.m_ibh);
		bgfx::setVertexBuffer(0, group.m_vbh);

		if (bgfx::isValid(texColor)) bgfx::setTexture(0, sTexColor, texColor);
		if (bgfx::isValid(texNormal)) bgfx::setTexture(1, sTexNormal, texNormal);

		bgfx::submit(id, program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
	}

	bgfx::discard();
}

void Mesh::Submit(const MeshState* const* _state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const
{
	const uint32_t cached = bgfx::setTransform(mtx, numMatrices);

	for (uint32_t pass = 0; pass < numPasses; ++pass)
	{
		bgfx::setTransform(cached, numMatrices);

		const MeshState& state = *_state[pass];
		bgfx::setState(state.m_state);

		for (uint8_t tex = 0; tex < state.m_numTextures; ++tex)
		{
			const MeshState::Texture& texture = state.m_textures[tex];
			bgfx::setTexture(texture.m_stage, texture.m_sampler, texture.m_texture, texture.m_flags);
		}

		for (const auto& group : groups)
		{
			bgfx::setIndexBuffer(group.m_ibh);
			bgfx::setVertexBuffer(0, group.m_vbh);
			bgfx::submit(state.m_viewId, state.m_program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
		}

		bgfx::discard(0 | BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE | BGFX_DISCARD_TRANSFORM);
	}

	bgfx::discard();
}

void Mesh::SynchronizePhysics()
{
	if (type & ObjectType_Group)
	{
		for (auto& child : children)
			child->SynchronizePhysics();
	}
	// physics?
	else if (body && body->enabled && body->mass >= 0.0f)
	{
		btTransform bTransform;
		auto        motionState = body->body->getMotionState();
		motionState->getWorldTransform(bTransform);

		float matrix[16];
		bTransform.getOpenGLMatrix(matrix);
		matrixWorld = glm::make_mat4(matrix) * scaleMatrix;
	}
	// interpolation
	else if (posTs > 0.0 || quatTs > 0.0)
	{
		const double interval = xsettings.repeatInterval * 1e-3;
		const double nowd     = Nowd();

		if (posTs > 0.0)
		{
			if (const double elapsed  = nowd - posTs; elapsed < interval)
				position = glm::mix(position1, position2, TO_FLOAT(elapsed / interval));
			else
			{
				position = position2;
				posTs    = 0;
			}
		}
		if (quatTs > 0.0)
		{
			if (const double elapsed  = nowd - quatTs; elapsed < interval)
				quaternion = glm::slerp(quaternion1, quaternion2, TO_FLOAT(elapsed / interval));
			else
			{
				quaternion = quaternion2;
				quatTs     = 0;
			}
		}
		UpdateLocalMatrix("SynchronizePhysics");
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

static sMesh MeshLoad(std::string_view name, bx::ReaderSeekerI* reader, bool ramcopy)
{
	auto mesh = std::make_shared<Mesh>(name);
	mesh->Load(reader, ramcopy);
	return mesh;
}

sMesh MeshLoad(std::string_view name, const bx::FilePath& filePath, bool ramcopy)
{
	bx::FileReaderI* reader = entry::getFileReader();
	if (bx::open(reader, filePath))
	{
		auto mesh = MeshLoad(name, reader, ramcopy);
		bx::close(reader);
		return mesh;
	}
	return nullptr;
}

MeshState* MeshStateCreate()
{
	MeshState* state = (MeshState*)bx::alloc(entry::getAllocator(), sizeof(MeshState));
	return state;
}

void MeshStateDestroy(MeshState* meshState)
{
	bx::free(entry::getAllocator(), meshState);
}
