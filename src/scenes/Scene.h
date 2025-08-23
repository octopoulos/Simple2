// Scene.h
// @author octopoulos
// @version 2025-08-19

#pragma once

#include "objects/Object3d.h"

class Scene : public Object3d
{
public:
	Scene()
	    : Object3d("Scene", ObjectType_Scene)
	{
	}

	~Scene() = default;

	/// Clear the scene, but keep: camera/cursor/map
	void Clear();
};
