// Scene.h
// @author octopoulos
// @version 2025-08-05

#pragma once

#include "objects/Object3d.h"

class Scene : public Object3d
{
public:
	UMAP_STR<std::weak_ptr<Object3d>> names = {}; ///< named children

	Scene()
	{
		name = "Scene";
		type = ObjectType_Scene;
	}

	~Scene() = default;

	/// Add a child and map its name
	void AddChild(sObject3d child) override;

	/// Clear the scene
	void Clear();

	/// Find an direct child by name
	Object3d* GetObjectByName(std::string_view name) const;

	/// Open a scene file + replace the scene
	bool OpenScene(const std::filesystem::path& filename);

	/// Remove a child and unmap its name
	void RemoveChild(const sObject3d& child) override;

	/// Save the scene to a file
	bool SaveScene(const std::filesystem::path& filename);
};
