// Mesh.cpp
// @author octopoulos
// @version 2025-09-07

#include "stdafx.h"
#include "objects/Mesh.h"
//
#include "core/common3d.h"  // GlmToBullet
#include "loaders/writer.h" // WRITE_KEY_xxx
#include "ui/ui.h"          // ui::
#include "ui/xsettings.h"   // xsettings

constexpr uint64_t defaultState = 0
    //| BGFX_STATE_CULL_CCW
    | BGFX_STATE_DEPTH_TEST_LESS
    | BGFX_STATE_MSAA
    | BGFX_STATE_WRITE_A
    | BGFX_STATE_WRITE_RGB
    | BGFX_STATE_WRITE_Z;

void Mesh::ActivatePhysics(bool activate)
{
	body->enabled = activate;
	if (auto& sbody = body->body)
	{
		sbody->setActivationState(activate ? ACTIVE_TAG : DISABLE_SIMULATION);
		sbody->activate(activate);
	}
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

void Mesh::CreateShapeBody(PhysicsWorld* physics, int shapeType, float mass, const btVector4& newDims)
{
	body = std::make_unique<Body>(physics);
	body->CreateShape(shapeType, this, newDims);
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
						memcpy(mtx, glm::value_ptr(child->matrixWorld), 64);

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

int Mesh::Serialize(fmt::memory_buffer& outString, int depth, int bounds, bool addChildren) const
{
	// skip some objects
	if (type & (ObjectType_Cursor)) return -1;

	int keyId = Object3d::Serialize(outString, depth, (bounds & 1) ? 1 : 0, addChildren && load == MeshLoad_None);
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
	if (load != MeshLoad_None) WRITE_KEY_INT(load);
	if (load != MeshLoad_Full)
	{
		if (const auto material1 = material0 ? material0 : material)
		{
			WRITE_KEY("material");
			material1->Serialize(outString, depth);
		}
	}
	if (modelName.size()) WRITE_KEY_STRING(modelName);

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Mesh::SetBodyTransform()
{
	position = matrixWorld[3];

	PrintMatrix(matrixWorld, "SBT/1");

	for(int i = 0; i < 3; i++)
	{
		scale[i] = glm::length(glm::vec3(matrixWorld[i]));
		// if (!scale[i]) scale[i] = 1.0f;
	}

	ui::Log("scale={} {} {}", scale[0], scale[1], scale[2]);

	const glm::mat3 rotMtx(
		glm::vec3(matrixWorld[0]) / scale[0],
		glm::vec3(matrixWorld[1]) / scale[1],
		glm::vec3(matrixWorld[2]) / scale[2]);

	quaternion  = glm::quat_cast(rotMtx);
	rotation    = glm::eulerAngles(quaternion);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	ui::Log("quaternion={} {} {} {}", quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
	ui::Log("rotation={} {} {}", rotation[0], rotation[1], rotation[2]);
	UpdateLocalMatrix("SetBodyTransform");

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(position.x, position.y, position.z));
	transform.setRotation(btQuaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w));

	// update transform
	if (auto& sbody = body->body)
	{
		sbody->setWorldTransform(transform);
		if (auto* motionState = sbody->getMotionState())
			motionState->setWorldTransform(transform);
	}
}

void Mesh::ShowSettings(bool isPopup, int show)
{
	int mode = 3;
	if (isPopup) mode |= 4;

	// name + transform
	if (show & (ShowObject_Basic | ShowObject_Transform))
		Object3d::ShowSettings(isPopup, show);

	// geometry
	if (show & ShowObject_Geometry)
	{
		if (geometry) geometry->ShowSettings(isPopup, show);
	}

	// physics
	if (show & ShowObject_Physics)
	{
		if (body) body->ShowSettings(isPopup, show);
	}

	// material
	if (show & (ShowObject_MaterialShaders | ShowObject_MaterialTextures))
	{
		if (material0)
			material0->ShowSettings(isPopup, show);
		else if (material)
			material->ShowSettings(isPopup, show);
	}
}

void Mesh::ShowTable() const
{
	Object3d::ShowTable();

	// clang-format off
	ui::ShowTable({
		{ "groups.size", std::to_string(groups.size()) },
		{ "load"       , std::to_string(load)          },
		{ "modelName"  , modelName                     },
	});
	// clang-format on

	if (body) body->ShowTable();
	if (geometry) geometry->ShowTable();
	if (material0)
		material0->ShowTable();
	else if (material)
		material->ShowTable();
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

	// physics?
	if (body && body->body && body->enabled && body->mass >= 0.0f)
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

		// Object3d::SynchronizePhysics();
		UpdateWorldMatrix(false);
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
	else if (type & ObjectType_Group)
	{
		Object3d::SynchronizePhysics();
	}

	return change;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

MeshState* MeshStateCreate()
{
	MeshState* state = (MeshState*)bx::alloc(entry::getAllocator(), sizeof(MeshState));
	return state;
}

void MeshStateDestroy(MeshState* meshState)
{
	bx::free(entry::getAllocator(), meshState);
}
