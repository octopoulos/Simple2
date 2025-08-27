// Object3d.cpp
// @author octopoulos
// @version 2025-08-23

#include "stdafx.h"
#include "objects/Object3d.h"
//
#include "common/config.h"
#include "core/common3d.h"
#include "loaders/writer.h"
#include "ui/ui.h"
#include "ui/xsettings.h"

// clang-format off
static const MAP_INT_STR objectTypeNames = {
	{ ObjectType_Basic    , "Basic"     },
	{ ObjectType_Camera   , "Camera"    },
	{ ObjectType_Clone    , "Clone"     },
	{ ObjectType_Cursor   , "Cursor"    },
	{ ObjectType_Group    , "Group"     },
	{ ObjectType_HasBody  , "HasBody"   },
	{ ObjectType_Instance , "Instance"  },
	{ ObjectType_Map      , "Map"       },
	{ ObjectType_Mesh     , "Mesh"      },
	{ ObjectType_RubikCube, "RubikCube" },
	{ ObjectType_Scene    , "Scene"     },
};
// clang-format on

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBJECT3D
///////////

void Object3d::AddChild(sObject3d child)
{
	type |= ObjectType_Group;

	if (child->name.size())
	{
		const auto& [it, inserted] = names.try_emplace(child->name, child);
		if (!inserted) ui::LogWarning("AddChild: {} already exists", child->name);
	}

	child->id     = TO_INT(children.size());
	child->parent = this;
	children.push_back(std::move(child));
}

sObject3d Object3d::GetObjectByName(std::string_view name) const
{
	if (const auto& it = names.find(name); it != names.end())
	{
		if (auto sp = it->second.lock()) return sp;
	}
	return nullptr;
}

void Object3d::RemoveChild(const sObject3d& child)
{
	if (child && child->name.size())
		if (const auto& it = names.find(child->name); it != names.end())
			names.erase(it);

	if (const auto& it = std::find(children.begin(), children.end(), child); it != children.end())
	{
		(*it)->parent = nullptr;
		children.erase(it);
	}
}

void Object3d::Render(uint8_t viewId, int renderFlags)
{
	if (!visible) return;

	if (type & (ObjectType_Group | ObjectType_Scene))
	{
		for (const auto& child : children)
			child->Render(viewId, renderFlags);
	}
}

void Object3d::RotationFromIrot(bool instant)
{
	if (DEV_rotate) ui::Log("RotationFromIrot: {} {} : {} {} {}", name, instant, irot[0], irot[1], irot[2]);
	// normalize irot to [-180, 180] degrees
	const int snap = xsettings.angleInc;
	for (int i = 0; i < 3; ++i)
	{
		const float degrees = TO_INT(bx::mod(TO_FLOAT(irot[i]) + 180.0f, 360.0f) - 180.0f);
		irot[i] = TO_INT(round(degrees / snap) * snap);
	}

	rotation = glm::vec3(bx::toRad(TO_FLOAT(irot[0])), bx::toRad(TO_FLOAT(irot[1])), bx::toRad(TO_FLOAT(irot[2])));
	if ((!instant || true) && xsettings.smoothQuat)
	{
		quaternion1 = quaternion;
		quaternion2 = glm::quat(rotation);
		quatTs      = Nowd();
	}
	else
	{
		quaternion = glm::quat(rotation);
		quatTs     = 0.0;
	}
}

void Object3d::ScaleIrotPosition(const glm::vec3& _scale, const std::array<int, 3>& _irot, const glm::vec3& _position)
{
	std::memcpy(irot, _irot.data(), sizeof(irot));

	position    = _position;
	scale       = _scale;
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	RotationFromIrot(true);
	UpdateLocalMatrix("ScaleIrotPosition");
}

void Object3d::ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position)
{
	position    = _position;
	rotation    = _rotation;
	scale       = _scale;
	quaternion  = glm::quat(rotation);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	irot[0] = TO_INT(bx::toDeg(rotation.x));
	irot[1] = TO_INT(bx::toDeg(rotation.y));
	irot[2] = TO_INT(bx::toDeg(rotation.z));

	UpdateLocalMatrix("ScaleRotationPosition");
}

void Object3d::ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position)
{
	position    = _position;
	quaternion  = _quaternion;
	scale       = _scale;
	rotation    = glm::eulerAngles(quaternion);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	irot[0] = TO_INT(bx::toDeg(rotation.x));
	irot[1] = TO_INT(bx::toDeg(rotation.y));
	irot[2] = TO_INT(bx::toDeg(rotation.z));

	UpdateLocalMatrix("ScaleQuaternionPosition");
}

int Object3d::Serialize(fmt::memory_buffer& outString, int depth, int bounds) const
{
	// skip Scene.groups except Map
	if (depth == 1 && (type & ObjectType_Group) && !(type & ObjectType_Map)) return -1;

	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	if (irot[0] || irot[1] || irot[2]) WRITE_KEY_INT3(irot);
	// if (!(type & ObjectType_HasBody) && matrix != glm::mat4(1.0f)) WRITE_KEY_MATRIX(matrix);
	// if (matrixWorld != glm::mat4(1.0f)) WRITE_KEY_MATRIX(matrixWorld);
	WRITE_KEY_STRING(name);
	WRITE_KEY_VEC3(position);
	if (scale != glm::vec3(1.0f)) WRITE_KEY_VEC3(scale);
	WRITE_KEY_STRING2("type", ObjectName(type));
	if (!visible) WRITE_KEY_BOOL(visible);

	// save children
	if (children.size())
	{
		WRITE_KEY("children");
		WRITE_CHAR('[');
		bool lastSaved = false;
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (lastSaved) WRITE_CHAR(',');
			lastSaved = (children[i]->Serialize(outString, depth + 1, 3) > -1);
		}
		if (outString[outString.size() - 1] == ',') outString.resize(outString.size() - 1);
		WRITE_CHAR(']');
	}

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Object3d::ShowTable()
{
	// clang-format off
	ui::ShowTable({
		{ "id"        , std::to_string(id)                                                                          },
		{ "irot"      , fmt::format("{}:{}:{}", irot[0], irot[1], irot[2])                                          },
		{ "name"      , name                                                                                        },
		{ "names"     , std::to_string(names.size())                                                                },
		{ "position"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", position.x, position.y, position.z)                     },
		{ "position1" , fmt::format("{:.2f}:{:.2f}:{:.2f}", position1.x, position1.y, position1.z)                  },
		{ "position2" , fmt::format("{:.2f}:{:.2f}:{:.2f}", position2.x, position2.y, position2.z)                  },
		{ "posTs"     , std::to_string(posTs)                                                                       },
		{ "quaternion", fmt::format("{:.2f}:{:.2f}:{:.2f}", quaternion.x, quaternion.y, quaternion.z, quaternion.w) },
		{ "quatTs"    , std::to_string(quatTs)                                                                      },
		{ "rotation"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", rotation.x, rotation.y, rotation.z)                     },
		{ "scale"     , fmt::format("{:.2f}:{:.2f}:{:.2f}", scale.x, scale.y, scale.z)                              },
		{ "type"      , std::to_string(type)                                                                        },
		{ "visible"   , std::to_string(visible)                                                                     },
	});
	// clang-format on
}

void Object3d::SynchronizePhysics()
{
	if (type & ObjectType_Group)
	{
		for (auto& child : children)
			child->SynchronizePhysics();
	}
}

void Object3d::UpdateLocalMatrix(std::string_view origin)
{
	matrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	UpdateWorldMatrix(true);
	if (DEV_matrix)
	{
		ui::Log("UpdateLocalMatrix/{}: {} {} {} {} : {} {} {}", origin, name, position.x, position.y, position.z, irot[0], irot[1], irot[2]);
		PrintMatrix(matrixWorld, name);
	}
}

void Object3d::UpdateWorldMatrix(bool force)
{
	if (!(type & ObjectType_HasBody) || force)
	{
		if (parent && !(parent->type & ObjectType_Scene))
			matrixWorld = parent->matrixWorld * matrix;
		else
			matrixWorld = matrix;
	}

	for (auto& child : children)
		child->UpdateWorldMatrix();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

std::string ObjectName(int type)
{
	std::string result;
	for (const auto& [flag, name] : objectTypeNames)
	{
		if (type & flag)
		{
			if (result.size()) result += ' ';
			result += name;
		}
	}
	return result;
}

int ObjectType(std::string_view name)
{
	static UMAP_STR_INT objectNameTypes;
	if (objectNameTypes.empty())
	{
		for (const auto& [type, name] : objectTypeNames)
			objectNameTypes[name] = type;
	}

	int result = 0;
	SplitStringView(name, ' ', false, [&](std::string_view split, int) {
		result |= FindDefault(objectNameTypes, split, 0);
	});
	return result;
}
