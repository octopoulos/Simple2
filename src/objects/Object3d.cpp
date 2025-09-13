// Object3d.cpp
// @author octopoulos
// @version 2025-09-09

#include "stdafx.h"
#include "objects/Object3d.h"
//
#include "common/config.h"  // DEV_matrix, DEV_rotate
#include "core/common3d.h"  // PrintMatrix
#include "loaders/writer.h" // WRITE_INIT, WRITE_KEY_xxx
#include "ui/ui.h"          // ui::
#include "ui/xsettings.h"   // xsettings

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
	++childInc;

	ids.emplace(childInc, child);
	if (child->name.size())
	{
		const auto& [it, inserted] = names.try_emplace(child->name, child);
		if (!inserted) ui::LogWarning("AddChild: {} already exists", child->name);
	}

	child->id     = childInc;
	child->parent = this;
	children.push_back(std::move(child));
}

void Object3d::ClearDeads(bool force)
{
	// 1) collect
	std::vector<sObject3d> removes;
	for (auto& child : children)
	{
		if (child->dead & Dead_Remove) removes.push_back(child);
	}

	// 2) remove
	ui::Log("ClearDeads: removes={} ({})", removes.size(), name);
	for (auto& remove : removes) RemoveChild(remove);
}

void Object3d::DecomposeMatrix()
{
	// extract position (translation)
	position = glm::vec3(matrix[3]);

	// extract scale from basis vectors
	scale.x = glm::length(glm::vec3(matrix[0]));
	scale.y = glm::length(glm::vec3(matrix[1]));
	scale.z = glm::length(glm::vec3(matrix[2]));

	// avoid division by zero
	if (scale.x == 0.0f) scale.x = 1.0f;
	if (scale.y == 0.0f) scale.y = 1.0f;
	if (scale.z == 0.0f) scale.z = 1.0f;

	// extract rotation matrix by removing scale
	glm::mat3 rotationMatrix(
	    glm::vec3(matrix[0]) / scale.x,
	    glm::vec3(matrix[1]) / scale.y,
	    glm::vec3(matrix[2]) / scale.z);

	// convert to quaternion
	quaternion = glm::normalize(glm::quat_cast(rotationMatrix));
	RotationFromQuaternion();
}

sObject3d Object3d::GetObjectById(int id) const
{
	if (const auto& it = ids.find(id); it != ids.end())
	{
		if (auto sp = it->second.lock()) return sp;
	}
	return nullptr;
}

sObject3d Object3d::GetObjectByName(std::string_view name) const
{
	if (const auto& it = names.find(name); it != names.end())
	{
		if (auto sp = it->second.lock()) return sp;
	}
	return nullptr;
}

void Object3d::IrotFromRotation()
{
	irot[0] = TO_INT(bx::toDeg(rotation.x));
	irot[1] = TO_INT(bx::toDeg(rotation.y));
	irot[2] = TO_INT(bx::toDeg(rotation.z));
}

bool Object3d::RemoveChild(const sObject3d& child)
{
	if (!child) return false;

	// 1) ids
	if (const auto& it = ids.find(child->id); it != ids.end())
		ids.erase(it);

	// 2) names
	if (child->name.size())
		if (const auto& it = names.find(child->name); it != names.end())
			names.erase(it);

	// 3) children
	if (const auto& it = std::find(children.begin(), children.end(), child); it != children.end())
	{
		(*it)->parent = nullptr;
		children.erase(it);
		return true;
	}
	return false;
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
	if (!instant && xsettings.smoothQuat)
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

void Object3d::RotationFromQuaternion()
{
	rotation = glm::eulerAngles(quaternion);
	IrotFromRotation();
}

void Object3d::ScaleIrotPosition(const glm::vec3& _scale, const std::array<int, 3>& _irot, const glm::vec3& _position)
{
	memcpy(irot, _irot.data(), sizeof(irot));

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

	IrotFromRotation();
	UpdateLocalMatrix("ScaleRotationPosition");
}

void Object3d::ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position)
{
	position    = _position;
	quaternion  = _quaternion;
	scale       = _scale;
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	RotationFromQuaternion();
	UpdateLocalMatrix("ScaleQuaternionPosition");
}

int Object3d::Serialize(fmt::memory_buffer& outString, int depth, int bounds, bool addChildren) const
{
	// skip Scene.groups except Map
	if (depth == 1 && (type & ObjectType_Group) && !(type & ObjectType_Map)) return -1;
	if (placing) return -2;

	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	if (irot[0] || irot[1] || irot[2]) WRITE_KEY_INT3(irot);
	WRITE_KEY_STRING(name);
	WRITE_KEY_VEC3(position);
	if (scale != glm::vec3(1.0f)) WRITE_KEY_VEC3(scale);
	WRITE_KEY_STRING2("type", ObjectName(type));
	if (!visible) WRITE_KEY_BOOL(visible);

	// save children, except if it's a loaded model
	if (addChildren && children.size())
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

void Object3d::ShowSettings(bool isPopup, int show)
{
	int mode = 3;
	if (isPopup) mode |= 4;

	// name
	if (show & ShowObject_Basic)
		ui::AddInputText(mode | 16, ".name", "Name", 256, 0, &name);

	// transform
	if (show & ShowObject_Transform)
	{
		if (ui::AddDragFloat(mode, ".position", "Position", glm::value_ptr(position), 3, 0.1f))
			UpdateLocalMatrix("Position");
		if (xsettings.rotateMode == RotateMode_Quaternion)
		{
			if (ui::AddDragFloat(mode, ".quaternion", "Quaternion", glm::value_ptr(quaternion), 4))
				UpdateLocalMatrix("Quaternion");
		}
		else
		{
			if (ui::AddDragFloat(mode, ".rotation", "Rotation", glm::value_ptr(rotation), 3, 0.01f))
			{
				quaternion = glm::quat(rotation);
				UpdateLocalMatrix("Rotation");
			}
		}
		ui::AddCombo(mode | (isPopup ? 16 : 0), "rotateMode", "Mode");
		if (ui::AddDragFloat(mode, ".scale", "Scale", glm::value_ptr(scale), 3))
		{
			scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
			UpdateLocalMatrix("Scale");
		}
	}
}

void Object3d::ShowTable() const
{
	// clang-format off
	ui::ShowTable({
		{ "id"         , std::to_string(id)                                                                           },
		{ "irot"       , fmt::format("{}:{}:{}", irot[0], irot[1], irot[2])                                           },
		{ "matrix"     , fmt::format("{:.2f}:{:.2f}:{:.2f}", matrix[3][0], matrix[3][1], matrix[3][2])                },
		{ "matrixWorld", fmt::format("{:.2f}:{:.2f}:{:.2f}", matrixWorld[3][0], matrixWorld[3][1], matrixWorld[3][2]) },
		{ "name"       , name                                                                                         },
		{ "names"      , std::to_string(names.size())                                                                 },
		{ "position"   , fmt::format("{:.2f}:{:.2f}:{:.2f}", position.x, position.y, position.z)                      },
		{ "position1"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", position1.x, position1.y, position1.z)                   },
		{ "position2"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", position2.x, position2.y, position2.z)                   },
		{ "posTs"      , std::to_string(posTs)                                                                        },
		{ "quaternion" , fmt::format("{:.2f}:{:.2f}:{:.2f}", quaternion.x, quaternion.y, quaternion.z, quaternion.w)  },
		{ "quatTs"     , std::to_string(quatTs)                                                                       },
		{ "rotation"   , fmt::format("{:.2f}:{:.2f}:{:.2f}", rotation.x, rotation.y, rotation.z)                      },
		{ "scale"      , fmt::format("{:.2f}:{:.2f}:{:.2f}", scale.x, scale.y, scale.z)                               },
		{ "type"       , std::to_string(type)                                                                         },
		{ "type:name"  , ObjectName(type)                                                                             },
		{ "visible"    , std::to_string(visible)                                                                      },
	});
	// clang-format on
}

int Object3d::SynchronizePhysics()
{
	if (type & ObjectType_Group)
	{
		int changes = 0;
		for (auto& child : children)
			changes += child->SynchronizePhysics();

		if (changes)
		{
			ui::Log("SynchronizePhysics: changes={}", changes);
			ClearDeads(false);
		}
	}

	return 0;
}

glm::mat4 Object3d::TransformPosition(const glm::vec3& _position)
{
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), _position) * glm::mat4_cast(quaternion) * scaleMatrix;
	if (parent && !(parent->type & ObjectType_Scene))
		matrix = parent->matrixWorld * matrix;

	return matrix;
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
		child->UpdateWorldMatrix(force);
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
