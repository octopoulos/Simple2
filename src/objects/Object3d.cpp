// Object3d.cpp
// @author octopoulos
// @version 2025-08-05

#include "stdafx.h"
#include "objects/Object3d.h"
//
#include "loaders/writer.h"

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

	if (type & ObjectType_Group)
	{
		for (const auto& child : children)
			child->Render(viewId, renderFlags);
	}
}

void Object3d::ScaleRotationPosition(const glm::vec3& _scale, const glm::vec3& _rotation, const glm::vec3& _position)
{
	position    = _position;
	rotation    = _rotation;
	scale       = _scale;
	quaternion  = glm::quat(rotation);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	UpdateLocalMatrix();
}

void Object3d::ScaleQuaternionPosition(const glm::vec3& _scale, const glm::quat& _quaternion, const glm::vec3& _position)
{
	position    = _position;
	quaternion  = _quaternion;
	scale       = _scale;
	rotation    = glm::eulerAngles(quaternion);
	scaleMatrix = glm::scale(glm::mat4(1.0f), scale);

	UpdateLocalMatrix();
}

int Object3d::Serialize(fmt::memory_buffer& outString, int bounds) const
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
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

void Object3d::UpdateLocalMatrix()
{
	matrix = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(quaternion) * scaleMatrix;
	UpdateWorldMatrix();
}

void Object3d::UpdateWorldMatrix()
{
	if (!(type & ObjectType_HasBody))
	{
		if (parent)
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
