// Object3d.cpp
// @author octopoulos
// @version 2025-09-30

#include "stdafx.h"
#include "objects/Object3d.h"
//
#include "common/config.h"  // DEV_matrix, DEV_rotate
#include "core/common3d.h"  // DecomposeMatrix, PrintMatrix
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
	{ ObjectType_RubikNode, "RubikNode" },
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
		if (!inserted) ui::LogWarning("AddChild: %s already exists", Cstr(child->name));
	}

	child->id         = childInc;
	// child->parent     = Object3d::shared_from_this(); // BUG HERE FROM CUBIE (RubikCube::Initialize)
	child->parentLink = !(type & (ObjectType_Container | ObjectType_Instance));
	children.push_back(std::move(child));
}

void Object3d::ClearDeads(bool force)
{
	// 1) collect
	std::vector<sObject3d> removes;
	for (auto& child : children)
	{
		if (force || (child->dead & Dead_Remove)) removes.push_back(child);
	}

	// 2) remove
	ui::Log("ClearDeads: removes=%lld (%s)", removes.size(), Cstr(name));
	for (auto& remove : removes) RemoveChild(remove);
}

int Object3d::CompleteInterpolation(bool warp, std::string_view origin)
{
	if (DEV_interpolate) ui::Log("CompleteInterpolation/%s: axisTs=%f posTs=%f quatTs=%f", Cstr(origin), axisTs, posTs, quatTs ? Nowd() - quatTs : 0);

	int change = 0;
	if (axisTs > 0.0 || posTs > 0.0)
	{
		if (warp) position = position2;
		change |= 1;
	}
	if (quatTs > 0.0)
	{
		if (warp)
		{
			quaternion = quaternion2;
			RotationFromQuaternion();
		}
		change |= 2;
	}

	if (warp)
	{
		axisTs = 0.0;
		posTs  = 0.0;
		quatTs = 0.0;
	}

	for (auto& child : children)
		change |= (child->CompleteInterpolation(warp, "child") << 2);

	if (warp && change > 0) UpdateLocalMatrix("CompleteInterpolation");
	return change;
}

void Object3d::DecomposeMatrix()
{
	::DecomposeMatrix(matrix, position, quaternion, scale);
	RotationFromQuaternion();
}

float Object3d::EaseFunction(double td)
{
	const float t = TO_FLOAT(td);

	// clang-format off
	switch (GetEase())
	{
	case Ease_InOutCubic: return EaseInOutCubic(t);
	case Ease_InOutQuad : return EaseInOutQuad(t);
	case Ease_OutQuad   : return EaseOutQuad(t);
	default: return t;
	}
	// clang-format on
}

int Object3d::GetEase() const
{
	if (type & ObjectType_Cursor) return xsettings.cursorEase;
	if (type & (ObjectType_RubikCube | ObjectType_RubikNode)) return xsettings.rubikEase;
	return Ease_None;
}

double Object3d::GetInterval(bool recalculate) const
{
	// 1) manually set interval
	if (!recalculate && interval > 0.0) return interval;

	// 2) default interval
	if (type & (ObjectType_RubikCube | ObjectType_RubikNode)) return xsettings.rubikRepeat * 1e-3;
	return xsettings.keyRepeat * 1e-3;
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
		(*it)->parent = {};
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
	if (DEV_rotate) ui::Log("RotationFromIrot: %s %d : %d %d %d", Cstr(name), instant, irot[0], irot[1], irot[2]);
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
	if (quatTs > 0.0)
		rotation = glm::eulerAngles(quaternion2);
	else
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

void Object3d::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("Object3d");

	const auto sparent = parent.lock();

	// clang-format off
	ui::ShowTable({
		{ "id"         , std::to_string(id)                                                                   },
		{ "irot"       , Format("%d:%d:%d", irot[0], irot[1], irot[2])                                        },
		{ "matrix"     , Format("%.2f:%.2f:%.2f", matrix[3][0], matrix[3][1], matrix[3][2])                   },
		{ "matrixWorld", Format("%.2f:%.2f:%.2f", matrixWorld[3][0], matrixWorld[3][1], matrixWorld[3][2])    },
		{ "name"       , name                                                                                 },
		{ "names"      , std::to_string(names.size())                                                         },
		{ "parent"     , sparent ? sparent->name : ""                                                         },
		{ "parentLink" , BoolString(parentLink)                                                               },
		{ "position"   , Format("%.2f:%.2f:%.2f", position.x, position.y, position.z)                         },
		{ "position1"  , Format("%.2f:%.2f:%.2f", position1.x, position1.y, position1.z)                      },
		{ "position2"  , Format("%.2f:%.2f:%.2f", position2.x, position2.y, position2.z)                      },
		{ "posTs"      , std::to_string(posTs)                                                                },
		{ "quaternion" , Format("%.2f:%.2f:%.2f", quaternion.x, quaternion.y, quaternion.z, quaternion.w)     },
		{ "quaternion1", Format("%.2f:%.2f:%.2f", quaternion1.x, quaternion1.y, quaternion1.z, quaternion1.w) },
		{ "quaternion2", Format("%.2f:%.2f:%.2f", quaternion2.x, quaternion2.y, quaternion2.z, quaternion2.w) },
		{ "quatTs"     , std::to_string(quatTs)                                                               },
		{ "rotation"   , Format("%.2f:%.2f:%.2f", rotation.x, rotation.y, rotation.z)                         },
		{ "scale"      , Format("%.2f:%.2f:%.2f", scale.x, scale.y, scale.z)                                  },
		{ "type"       , std::to_string(type)                                                                 },
		{ "type:name"  , ObjectName(type)                                                                     },
		{ "visible"    , std::to_string(visible)                                                              },
	});
	// clang-format on
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

int Object3d::SynchronizePhysics()
{
	if (type & ObjectType_Group)
	{
		int changes = 0;
		for (auto& child : children)
			changes += child->SynchronizePhysics();

		if (changes)
		{
			ui::Log("SynchronizePhysics: changes=%d", changes);
			ClearDeads(false);
		}
	}

	return 0;
}

glm::mat4 Object3d::TransformPosition(const glm::vec3& _position)
{
	glm::mat4 matrix = glm::translate(glm::mat4(1.0f), _position) * glm::mat4_cast(quaternion) * scaleMatrix;
	if (auto sparent = parent.lock(); sparent && !(sparent->type & ObjectType_Scene))
		matrix = sparent->matrixWorld * matrix;

	return matrix;
}

void Object3d::UpdateLocalMatrix(std::string_view origin)
{
	matrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	UpdateWorldMatrix(true);
	if (DEV_matrix)
	{
		ui::Log("UpdateLocalMatrix/%s: %s %f %f %f : %d %d %d", Cstr(origin), Cstr(name), position.x, position.y, position.z, irot[0], irot[1], irot[2]);
		PrintMatrix(matrixWorld, name);
	}
}

void Object3d::UpdateWorldMatrix(bool force)
{
	if (!(type & ObjectType_HasBody) || force)
	{
		if (auto sparent = parent.lock(); sparent && !(sparent->type & ObjectType_Scene) && parentLink)
			matrixWorld = sparent->matrixWorld * matrix;
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
