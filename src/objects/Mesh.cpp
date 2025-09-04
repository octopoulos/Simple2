// Mesh.cpp
// @author octopoulos
// @version 2025-08-30

#include "stdafx.h"
#include "objects/Mesh.h"
//
#include "loaders/writer.h" // WRITE_KEY_xxx
#include "ui/ui.h"          // ui::
#include "ui/xsettings.h"   // xsettings

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
			BGFX_DESTROY(group.ibh);
			BGFX_DESTROY(group.vbh);

			if (group.vertices) bx::free(allocator, group.vertices);
			if (group.indices) bx::free(allocator, group.indices);
		}
		groups.clear();
	}
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
			read(reader, group.sphere, &err);
			read(reader, group.aabb  , &err);
			read(reader, group.obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			read(reader, group.numVertices, &err);
			const bgfx::Memory* mem = bgfx::alloc(group.numVertices * stride);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.vertices = (uint8_t*)bx::alloc(allocator, group.numVertices * stride);
				bx::memCopy(group.vertices, mem->data, mem->size);
			}

			group.vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkVertexBufferCompressed:
		{
			// clang-format off
			read(reader, group.sphere, &err);
			read(reader, group.aabb  , &err);
			read(reader, group.obb   , &err);
			// clang-format on

			read(reader, layout, &err);

			const uint16_t stride = layout.getStride();

			read(reader, group.numVertices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numVertices * stride);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedVertices = bx::alloc(allocator, compressedSize);
			bx::read(reader, compressedVertices, compressedSize, &err);

			meshopt_decodeVertexBuffer(mem->data, group.numVertices, stride, (uint8_t*)compressedVertices, compressedSize);

			bx::free(allocator, compressedVertices);

			if (ramcopy)
			{
				group.vertices = (uint8_t*)bx::alloc(allocator, group.numVertices * stride);
				bx::memCopy(group.vertices, mem->data, mem->size);
			}
			group.vbh = bgfx::createVertexBuffer(mem, layout);
		}
		break;

		case kChunkIndexBuffer:
		{
			read(reader, group.numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numIndices * 2);
			read(reader, mem->data, mem->size, &err);

			if (ramcopy)
			{
				group.indices = (uint32_t*)bx::alloc(allocator, group.numIndices * 2);
				bx::memCopy(group.indices, mem->data, mem->size);
			}
			group.ibh = bgfx::createIndexBuffer(mem);
		}
		break;

		case kChunkIndexBufferCompressed:
		{
			bx::read(reader, group.numIndices, &err);

			const bgfx::Memory* mem = bgfx::alloc(group.numIndices * 2);

			uint32_t compressedSize;
			bx::read(reader, compressedSize, &err);

			void* compressedIndices = bx::alloc(allocator, compressedSize);

			bx::read(reader, compressedIndices, compressedSize, &err);

			meshopt_decodeIndexBuffer(mem->data, group.numIndices, 2, (uint8_t*)compressedIndices, compressedSize);

			bx::free(allocator, compressedIndices);

			if (ramcopy)
			{
				group.indices = (uint32_t*)bx::alloc(allocator, group.numIndices * 2);
				bx::memCopy(group.indices, mem->data, mem->size);
			}
			group.ibh = bgfx::createIndexBuffer(mem);
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
				read(reader, prim.startIndex , &err);
				read(reader, prim.numIndices , &err);
				read(reader, prim.startVertex, &err);
				read(reader, prim.numVertices, &err);
				read(reader, prim.sphere     , &err);
				read(reader, prim.aabb       , &err);
				read(reader, prim.obb        , &err);
				// clang-format on

				group.prims.push_back(prim);
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

void Mesh::Render(uint8_t viewId, int renderFlags)
{
	if (!visible) return;

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
						material->Apply();
						bgfx::setState(material->state ? material->state : defaultState);
						bgfx::submit(viewId, material->program);
					}
					else if (bgfx::isValid(material->program))
					{
						for (const auto& group : groups)
						{
							bgfx::setIndexBuffer(group.ibh);
							bgfx::setVertexBuffer(0, group.vbh);
							bgfx::setInstanceDataBuffer(&idb);
							material->Apply();
							bgfx::setState(material->state ? material->state : defaultState);
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
			bgfx::setState(material->state ? material->state : defaultState);
			bgfx::submit(viewId, material->program);
		}
		else if (bgfx::isValid(material->program))
		{
			Submit(viewId, material->program, glm::value_ptr(matrixWorld), material->state ? material->state : defaultState);
		}
	}
}

int Mesh::Serialize(fmt::memory_buffer& outString, int depth, int bounds) const
{
	// skip some objects
	if (type & (ObjectType_Cursor)) return -1;

	int keyId = Object3d::Serialize(outString, depth, (bounds & 1) ? 1 : 0);
	if (keyId < 0) return keyId;

	if (body)
	{
		WRITE_KEY("body");
		body->Serialize(outString, depth);
	}
	if (geometry)
	{
		WRITE_KEY("geometry");
		geometry->Serialize(outString, depth);
	}
	if (load) WRITE_KEY_INT(load);
	if (const auto material1 = material0 ? material0 : material)
	{
		WRITE_KEY("material");
		material1->Serialize(outString, depth);
	}
	if (modelName.size()) WRITE_KEY_STRING(modelName);

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

void Mesh::ShowTable() const
{
	Object3d::ShowTable();
	if (body) body->ShowTable();

	// clang-format off
	ui::ShowTable({
		{ "groups.size"   , std::to_string(groups.size())                    },
		{ "load"          , std::to_string(load)                             },
		{ "material.state", material ? std::to_string(material->state) : "?" },
		{ "modelName"     , modelName                                        },
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
		bgfx::setIndexBuffer(group.ibh);
		bgfx::setVertexBuffer(0, group.vbh);
		material->Apply();
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
		bgfx::setState(state.state);

		for (uint8_t tex = 0; tex < state.numTextures; ++tex)
		{
			const MeshState::Texture& texture = state.textures[tex];
			bgfx::setTexture(texture.stage, texture.sampler, texture.texture, texture.flags);
		}

		for (const auto& group : groups)
		{
			bgfx::setIndexBuffer(group.ibh);
			bgfx::setVertexBuffer(0, group.vbh);
			bgfx::submit(state.viewId, state.program, 0, BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_VERTEX_STREAMS);
		}

		bgfx::discard(0 | BGFX_DISCARD_BINDINGS | BGFX_DISCARD_STATE | BGFX_DISCARD_TRANSFORM);
	}

	bgfx::discard();
}

int Mesh::SynchronizePhysics()
{
	int change = 0;

	if (type & ObjectType_Group)
	{
		Object3d::SynchronizePhysics();
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

		// remove object if drops too low
		const auto& pos = bTransform.getOrigin();
		position        = glm::vec3(pos.x(), pos.y(), pos.z());
		// const auto& rot = bTransform.getRotation();
		// quaternion      = glm::quat(rot.w(), rot.x(), rot.y(), rot.z());
		if (position.y < xsettings.bottom)
		{
			dead   = Dead_Remove;
			change = 1;
		}
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

	return change;
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
