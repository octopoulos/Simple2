// Scene.h
// @author octopoulos
// @version 2025-10-14

#pragma once

#include "objects/Object3d.h"

class Scene : public Object3d
{
public:
	Scene()
	    : Object3d("Scene", ObjectType_Scene)
	{
	}

	~Scene() { ui::Log("~Scene"); }

	/// Clear the scene, but keep: camera/cursor/map
	void Clear();

	/// Get an Object3d as a Scene
	static std::shared_ptr<Scene> SharedPtr(const sObject3d& object)
	{
		return (object && (object->type & ObjectType_Scene)) ? std::static_pointer_cast<Scene>(object) : nullptr;
	}
};
