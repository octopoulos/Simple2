// Scene.cpp
// @author octopoulos
// @version 2025-08-19

#include "stdafx.h"
#include "app/App.h"
#include "scenes/Scene.h"
//
#include "core/Camera.h"
#include "loaders/MeshLoader.h"
#include "objects/Mesh.h"
#include "ui/xsettings.h"

#include <simdjson.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APP
//////

/// Add to recent files
/// - this means a file was just loaded/saved
static void AddRecent(const std::filesystem::path& filename)
{
	str2k temp;
	strcpy(temp, filename.string().c_str());

	auto& files = xsettings.recentFiles;
	int   id    = 0;
	while (id < 6 && strcmp(files[id], temp) != 0)
		++id;

	if (id > 0)
	{
		for (int i = std::min(5, id); i > 0; --i)
			strcpy(files[i], files[i - 1]);
		strcpy(files[0], temp);
	}
}

static void ParseObject(simdjson::ondemand::object& doc, sObject3d parent, void* physics)
{
	// 1) default values
	std::array<int, 3> irot     = {};
	std::string        name     = "";
	glm::vec3          position = glm::vec3(0.0f);
	glm::vec3          scale    = glm::vec3(1.0f);
	std::string        stype    = "";
	bool               tempBool = true;
	int                type     = ObjectType_Basic;
	bool               visible  = true;

	// 2) name / type / visible
	if (!doc["name"].get_string().get(name)) {}
	if (!doc["type"].get_string().get(stype)) type = ObjectType(stype);
	if (!doc["visible"].get_bool().get(tempBool)) visible = tempBool;

	// 3) irot
	simdjson::ondemand::array array;
	if (!doc["irot"].get_array().get(array))
	{
		for (int i = -1; auto val : array)
		{
			if (++i < 3)
			{
				int64_t value;
				if (!val.get_int64().get(value)) irot[i] = TO_INT(value);
			}
		}
	}

	// 4) position
	if (!doc["position"].get_array().get(array))
	{
		for (int i = -1; auto val : array)
		{
			if (++i < 3)
			{
				double value;
				if (!val.get_double().get(value)) position[i] = TO_FLOAT(value);
			}
		}
	}

	// 5) scale
	if (!doc["scale"].get_array().get(array))
	{
		for (int i = -1; auto val : array)
		{
			if (++i < 3)
			{
				double value;
				if (!val.get_double().get(value)) scale[i] = TO_FLOAT(value);
			}
		}
	}

	// 6) create object based on type
	sObject3d object = nullptr;
	if (type & ObjectType_Camera)
		object = std::make_shared<Camera>(name);
	else if (type & ObjectType_Mesh)
	{
		int64_t load = 0;
		if (!doc["load"].get_int64().get(load))
		{
			std::string modelName;
			if (!doc["modelName"].get_string().get(modelName))
				object = MeshLoader::LoadModelFull(name, modelName);
		}

		if (!object) object = std::make_shared<Mesh>(name, type);
		auto mesh = std::static_pointer_cast<Mesh>(object);

		// parse geometry/material/textures + physics ...
		simdjson::ondemand::object obj;
		if (!doc["material"].get_object().get(obj))
		{
			std::string fsName;
			std::string vsName;
			if (!obj["vsName"].get_string().get(vsName) && !obj["fsName"].get_string().get(fsName))
			{
				// mesh->geometry = CreateIcosahedronGeometry(3.0f, 8);
				// mesh->material = std::make_shared<Material>("vs_model_texture_normal", "fs_model_texture_normal");
				// mesh->LoadTextures("earth_day_4096.jpg", "earth_normal_2048.jpg");
			}
		}

		mesh->CreateShapeBody((PhysicsWorld*)physics, ShapeType_Box, 0.0f);
		mesh->geometry = CreateBoxGeometry();
	}
	else if (type & ObjectType_Scene)
		object = std::make_shared<Scene>();
	else
		object = std::make_shared<Object3d>(name, type);

	// 7) set properties
	object->visible = visible;
	object->ScaleIrotPosition(scale, irot, position);

	// 8) connect node
	parent->AddChild(object);

	// 8) check children
	if (!doc["children"].get_array().get(array))
	{
		for (simdjson::ondemand::object child : array)
			ParseObject(child, object, physics);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APP
//////

bool App::OpenScene(const std::filesystem::path& filename)
{
	simdjson::ondemand::parser parser;

	auto json = simdjson::padded_string::load(filename.string());

	simdjson::ondemand::document doc = parser.iterate(json);
	if (simdjson::ondemand::object obj; !doc.get_object().get(obj))
	{
		AddRecent(filename);
		ui::Log("Parsing JSON object from file: {}", filename.string());
		ParseObject(obj, scene, physics.get());
		return true;
	}

	return false;
}


bool App::SaveScene(const std::filesystem::path& filename)
{
	fmt::memory_buffer outString;
	scene->Serialize(outString);
	ui::Log("SaveScene: {}", OUTSTRING_VIEW);
	WriteData(filename, OUTSTRING_VIEW);
	AddRecent(filename);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCENE
////////

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

void Scene::RemoveChild(const sObject3d& child)
{
	if (child && child->name.size())
		if (const auto& it = names.find(child->name); it != names.end())
			names.erase(it);

	Object3d::RemoveChild(child);
}
