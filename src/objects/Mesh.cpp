// Mesh.cpp
// @author octopoulos
// @version 2025-10-04

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
	// 1) itself
	body->enabled = activate;
	if (auto& sbody = body->body)
	{
		sbody->setActivationState(activate ? ACTIVE_TAG : DISABLE_SIMULATION);
		sbody->activate(activate);
	}

	// 2) children
	for (auto& child : children)
	{
		if (auto mesh = Mesh::SharedPtr(child, ObjectType_HasBody))
			mesh->ActivatePhysics(activate);
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

void Mesh::Controls(const sCamera& camera, int modifier, const bool* downs, bool* ignores, const bool* keys)
{
}

void Mesh::CreateShapeBody(PhysicsWorld* physics, int shapeType, float mass, const btVector4& newDims)
{
	// 1) create shape
	body = std::make_unique<Body>(physics);
	body->CreateShape(shapeType, this, newDims);

	// 2) create physical body
	// child => get the position and rotation from the matrix
	if (auto sparent = parent.lock(); sparent && !(sparent->type & ObjectType_Container))
	{
		glm::vec3 positionW;
		glm::quat quaternionW;
		glm::vec3 scaleW;
		::DecomposeMatrix(matrixWorld, positionW, quaternionW, scaleW);
		body->CreateBody(mass, GlmToBullet(positionW), GlmToBullet(quaternionW));
	}
	// quicker path
	else body->CreateBody(mass, GlmToBullet(position), GlmToBullet(quaternion));

	// 3) data for raycasting
	if (auto& sbody = body->body)
		sbody->setUserPointer(this);

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

void Mesh::Explode()
{
	ui::Log("Mesh:Explode");
	ActivatePhysics(true);
	for (auto& child : children)
	{
	}
}

double Mesh::GetInterval(bool recalculate) const
{
	// 1) manually set interval
	if (!recalculate && interval > 0.0) return interval;

	// 2) default interval
	const double baseInterval = Object3d::GetInterval(recalculate);
	return baseInterval / (1 + nextKeys.size());
}

bool Mesh::HasBody(bool checkEnabled) const
{
	if (!body || !body->body) return false;
	if (checkEnabled)
	{
		if (!body->enabled || body->mass < 0.0f) return false;
	}
	return true;
}

void Mesh::QueueKey(int key, bool isQueue)
{
	if (isQueue)
		nextKeys.push_front(key);
	else
		nextKeys.push_back(key);
}

void Mesh::Render(uint8_t viewId, int renderFlags)
{
	if (!visible) return;

	if (type & ObjectType_Group)
	{
		if (const uint32_t numChild = TO_UINT32(children.size()))
		{
			if ((type & ObjectType_Instance) && (renderFlags & RenderFlag_Instancing))
			{
				// 64 bytes for 4x4 matrix
				const uint16_t stride = 64 + 16;

				// figure out how big of a buffer is available
				const uint32_t drawnChild = bgfx::getAvailInstanceDataBuffer(numChild, stride);
				const auto     missing    = numChild - drawnChild;
				//ui::Log("numChild=%d missing=%d layout=%d", numChild, missing, layout.getStride());

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
	glm::vec3 positionW;
	glm::quat quaternionW;
	glm::vec3 scaleW;
	::DecomposeMatrix(matrixWorld, positionW, quaternionW, scaleW);

	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(positionW.x, positionW.y, positionW.z));
	transform.setRotation(btQuaternion(quaternionW.x, quaternionW.y, quaternionW.z, quaternionW.w));

	// update transform
	if (auto& sbody = body->body)
	{
		sbody->setWorldTransform(transform);
		if (auto* motionState = sbody->getMotionState())
			motionState->setWorldTransform(transform);
	}
}

void Mesh::SetPhysics(PhysicsWorld* physics)
{
}

void Mesh::ShowInfoTable(bool showTitle) const
{
	Object3d::ShowInfoTable(showTitle);
	if (showTitle) ImGui::TextUnformatted("Mesh");

	// clang-format off
	ui::ShowTable({
		{ "aabb.max"     , FormatStr("%.2f %.2f %.2f", aabb.max.x, aabb.max.y, aabb.max.z)                },
		{ "aabb.min"     , FormatStr("%.2f %.2f %.2f", aabb.min.x, aabb.min.y, aabb.min.z)                },
		{ "groups.size"  , std::to_string(groups.size())                                                  },
		{ "load"         , std::to_string(load)                                                           },
		{ "modelName"    , modelName                                                                      },
		{ "nextKeys"     , std::to_string(nextKeys.size())                                                },
		{ "sphere.center", FormatStr("%.2f %.2f %.2f", sphere.center.x, sphere.center.y, sphere.center.z) },
		{ "sphere.radius", FormatStr("%.2f", sphere.radius)                                               },
	});
	// clang-format on

	if (body) body->ShowInfoTable();
	if (geometry) geometry->ShowInfoTable();
	if (material0)
		material0->ShowInfoTable();
	else if (material)
		material->ShowInfoTable();
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
	if (!parentLink && HasBody(true))
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

		UpdateWorldMatrix(false);
	}
	// interpolation
	else if (axisTs > 0.0 || posTs > 0.0 || quatTs > 0.0)
	{
		const double interval = GetInterval();
		const double nowd     = Nowd();

		// arc interpolation (overrides pos for layer turns)
		if (axisTs > 0.0)
		{
			if (const double elapsed = nowd - axisTs; elapsed < interval)
			{
				const auto axis = glm::slerp(axis1, axis2, EaseFunction(elapsed / interval));
				position        = axis * position1;
			}
			else
			{
				position = position2;
				axisTs   = 0.0;
				posTs    = 0.0;
			}
		}
		// linear position
		else if (posTs > 0.0)
		{
			if (const double elapsed  = nowd - posTs; elapsed < interval)
				position = glm::mix(position1, position2, EaseFunction(elapsed / interval));
			else
			{
				position = position2;
				posTs    = 0.0;
			}
		}
		// spherical interpolation
		if (quatTs > 0.0)
		{
			if (const double elapsed  = nowd - quatTs; elapsed < interval)
				quaternion = glm::slerp(quaternion1, quaternion2, EaseFunction(elapsed / interval));
			else
			{
				quaternion = quaternion2;
				quatTs     = 0.0;
			}
		}

		UpdateLocalMatrix("SynchronizePhysics");
		if (HasBody(false)) SetBodyTransform();
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
