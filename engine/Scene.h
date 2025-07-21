// Scene.h
// @author octopoulos
// @version 2025-07-16

#pragma once

class Scene : public Object3d
{
public:
	std::shared_ptr<Camera2>          camera = nullptr;
	UMAP_STR<std::weak_ptr<Object3d>> names  = {};

	Scene()  = default;
	~Scene() = default;

	void AddChild(sObject3d child) override
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

	void AddNamedChild(sObject3d child, std::string&& name)
	{
		child->name = std::move(name);
		AddChild(child);
	}

	void Clear()
	{
		children.clear();
		names.clear();
	}

	Object3d* GetObjectByName(std::string_view name) const
	{
		if (const auto& it = names.find(name); it != names.end())
		{
			if (auto sp = it->second.lock()) return sp.get();
		}
		return nullptr;
	}

	void RemoveChild(const sObject3d& child) override
	{
		if (child && child->name.size())
			if (const auto& it = names.find(child->name); it != names.end())
				names.erase(it);

		Object3d::RemoveChild(child);
	}

	void RenderScene(uint8_t viewId, uint16_t width, uint16_t height)
	{
		//if (camera)
		//{
		//	camera->aspect = height ? static_cast<float>(width) / height : 1.0f;
		//	camera->UpdateViewProjection(viewId);
		//}
		//bgfx::setViewRect(viewId, 0, 0, width, height);
		TraverseAndRender(viewId);
	}
};
