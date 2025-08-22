// Object3d.cpp
// @author octopoulos
// @version 2025-08-18

#include "stdafx.h"
#include "objects/Object3d.h"
//
#include "core/common3d.h"
#include "loaders/writer.h"
#include "ui/xsettings.h"

// clang-format off
static const MAP_INT_STR OBJECT_NAMES = {
	{ ObjectType_Basic    , "Basic"     },
	{ ObjectType_Camera   , "Camera"    },
	{ ObjectType_Clone    , "Clone"     },
	{ ObjectType_Group    , "Group"     },
	{ ObjectType_HasBody  , "HasBody"   },
	{ ObjectType_Instance , "Instance"  },
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
	child->parent = this;
	children.push_back(std::move(child));
}

void Object3d::RemoveChild(const sObject3d& child)
{
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
	ui::Log("RotationFromIrot: {} {} : {} {} {}", name, instant, irot[0], irot[1], irot[2]);
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
	ui::Log("ScaleIrotPosition: {}", name);
	UpdateLocalMatrix();
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

	ui::Log("ScaleRotationPosition: {}", name);
	UpdateLocalMatrix();
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

	ui::Log("ScaleQuaternionPosition: {}", name);
	UpdateLocalMatrix();
}

int Object3d::Serialize(fmt::memory_buffer& outString, int bounds) const
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	if (irot[0] || irot[1] || irot[2]) WRITE_KEY_INT3(irot);
	if (!(type & ObjectType_HasBody) && matrix != glm::mat4(1.0f)) WRITE_KEY_MATRIX(matrix);
	if (matrixWorld != glm::mat4(1.0f)) WRITE_KEY_MATRIX(matrixWorld);
	WRITE_KEY_STRING(name);
	WRITE_KEY_STRING2("type", ObjectName(type));
	WRITE_KEY_BOOL(visible);
	if (children.size())
	{
		WRITE_KEY("children");
		WRITE_CHAR('[');
		for (size_t i = 0; i < children.size(); ++i)
		{
			if (i > 0) WRITE_CHAR(',');
			children[i]->Serialize(outString, 3);
		}
		WRITE_CHAR(']');
	}
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Object3d::SynchronizePhysics()
{
}

void Object3d::UpdateLocalMatrix(bool force)
{
	matrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	UpdateWorldMatrix(force);
	ui::Log("UpdateLocalMatrix: {} {} {} {} : {} {} {}", name, position.x, position.y, position.z, irot[0], irot[1], irot[2]);
	PrintMatrix(matrixWorld, name);
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
	for (const auto& [flag, name] : OBJECT_NAMES)
	{
		if (type & flag)
		{
			if (result.size()) result += ' ';
			result += name;
		}
	}
	return result;
}
