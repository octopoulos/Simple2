// Scene.cpp
// @author octopoulos
// @version 2025-08-05

#include "stdafx.h"
#include "scenes/Scene.h"
//

void Scene::AddChild(sObject3d child)
{
	if (child->name.size())
	{
		const auto& [it, inserted] = names.try_emplace(child->name, child);
		if (!inserted) ui::LogWarning("Scene/AddChild: {} already exists", child->name);
	}

	child->id     = TO_INT(children.size());
	child->parent = this;
	children.push_back(std::move(child));
}

void Scene::Clear()
{
	children.clear();
	names.clear();
}

sObject3d Scene::GetObjectByName(std::string_view name) const
{
	if (const auto& it = names.find(name); it != names.end())
	{
		if (auto sp = it->second.lock()) return sp;
	}
	return nullptr;
}

bool Scene::OpenScene(const std::filesystem::path& filename)
{
	ui::Log("OpenScene: {}", filename);
	return true;
}

void Scene::RemoveChild(const sObject3d& child)
{
	if (child && child->name.size())
		if (const auto& it = names.find(child->name); it != names.end())
			names.erase(it);

	Object3d::RemoveChild(child);
}

bool Scene::SaveScene(const std::filesystem::path& filename)
{
	fmt::memory_buffer outString;
	Serialize(outString);
	ui::Log("SaveScene: {}", OUTSTRING_VIEW);
	WriteData(filename, OUTSTRING_VIEW);
	return true;
}
