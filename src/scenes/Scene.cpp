// Scene.cpp
// @author octopoulos
// @version 2025-08-20

#include "stdafx.h"
#include "scenes/Scene.h"
#include "app/App.h"
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

static void ParseObject(simdjson::ondemand::object& doc, sObject3d parent, sObject3d scene, void* physics)
{
	// 1) default values
	std::array<int, 3> irot       = {};
	std::string        name       = "";
	glm::vec3          position   = glm::vec3(0.0f);
	glm::vec3          scale      = glm::vec3(1.0f);
	int                type       = ObjectType_Basic;
	bool               visible    = true;

	// temp vars
	simdjson::ondemand::array  array      = {};
	simdjson::ondemand::object obj        = {};
	bool                       tempBool   = true;
	std::string                tempString = "";

	// 2) name / type / visible
	if (!doc["name"].get_string().get(name)) {}
	if (!doc["type"].get_string().get(tempString)) type = ObjectType(tempString);
	if (!doc["visible"].get_bool().get(tempBool)) visible = tempBool;

	// 3) irot
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
	sObject3d exist      = nullptr;
	sObject3d object     = nullptr;
	bool      positioned = false;

	if (type & ObjectType_Camera)
	{
		exist = scene->GetObjectByName("Camera");
		if (exist) object = exist;
	}
	else if (type & ObjectType_Cursor)
	{
		exist = scene->GetObjectByName("Cursor");
		if (exist) object = exist;
	}
	else if (type & ObjectType_Map)
	{
		exist = scene->GetObjectByName("Map");
		if (exist) object = exist;
	}
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

		// geometry
		if (!doc["geometry"].get_object().get(obj))
		{
			std::string args;
			if (!obj["args"].get_string().get(args)) {}
			if (!obj["type"].get_string().get(tempString))
			{
				const int geometryType = GeometryType(tempString);
				mesh->geometry = CreateAnyGeometry(geometryType, args);
			}
		}

		// material
		if (!doc["material"].get_object().get(obj))
		{
			std::string fsName;
			std::string vsName;
			if (!obj["fsName"].get_string().get(fsName) && !obj["vsName"].get_string().get(vsName))
				mesh->material = std::make_shared<Material>(vsName, fsName);
		}

		// state
		{
			int64_t state;
			if (!doc["state"].get_int64().get(state))
				mesh->state = state;
		}

		// textures
		if (!doc["texNames"].get_array().get(array))
		{
			std::string colorName;
			std::string normalName;
			for (int i = -1; std::string_view texName : array)
			{
				++i;
				switch (i)
				{
				case 0: colorName = texName; break;
				case 1: normalName = texName; break;
				}
			}
			mesh->LoadTextures(colorName, normalName);
		}

		// body
		{
			double mass      = 0.0;
			int    shapeType = ShapeType_Box;

			if (!doc["body"].get_object().get(obj))
			{
				if (!obj["mass"].get_double().get(mass)) {}
				if (!obj["shapeType"].get_string().get(tempString)) shapeType = ShapeType(tempString);
			}

			mesh->ScaleIrotPosition(scale, irot, position);
			positioned = true;
			mesh->CreateShapeBody((PhysicsWorld*)physics, shapeType, TO_FLOAT(mass));
		}
	}
	else if (type & ObjectType_Scene)
	{
		exist = scene;
		if (exist) object = exist;
	}
	else object = std::make_shared<Object3d>(name, type);

	// 7) set properties
	if (object)
	{
		object->visible = visible;
		if (!positioned) object->ScaleIrotPosition(scale, irot, position);
	}

	// 8) connect node
	if (object && !exist)
		parent->AddChild(object);

	// 8) check children
	if (!doc["children"].get_array().get(array))
	{
		for (simdjson::ondemand::object child : array)
			ParseObject(child, object ? object : parent, scene, physics);
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
		std::static_pointer_cast<Scene>(scene)->Clear();
		ParseObject(obj, scene, scene, physics.get());
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

void Scene::Clear()
{
	// 1) collect
	std::vector<sObject3d> removes;
	for (auto& child : children)
	{
		if (!(child->type & (ObjectType_Camera | ObjectType_Cursor | ObjectType_Map)))
			removes.push_back(child);
	}

	// 2) remove
	for (auto& remove : removes)
		RemoveChild(remove);
}
