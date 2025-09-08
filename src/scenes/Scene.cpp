// Scene.cpp
// @author octopoulos
// @version 2025-09-04

#include "stdafx.h"
#include "scenes/Scene.h"
#include "app/App.h"
//
#include "common/config.h"             // DEV_matrix
#include "core/common3d.h"             // PrintMatrix
#include "loaders/MeshLoader.h"        // MeshLoader::
#include "materials/MaterialManager.h" // GetMaterialManager
#include "objects/RubikCube.h"         // RubikCube

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
		for (int i = bx::min(5, id); i > 0; --i)
			strcpy(files[i], files[i - 1]);
		strcpy(files[0], temp);
	}
}

static void GetArrayFloat(simdjson::ondemand::object& doc, const char* key, float* out)
{
	simdjson::ondemand::array arr;
	if (!doc[key].get_array().get(arr))
	{
		for (int i = -1; auto val : arr)
		{
			if (++i >= 3) break;
			double value;
			if (!val.get_double().get(value)) out[i] = TO_FLOAT(value);
		}
	}
}

static void GetArrayInt(simdjson::ondemand::object& doc, const char* key, std::array<int, 3>& out)
{
	simdjson::ondemand::array arr;
	if (!doc[key].get_array().get(arr))
	{
		for (int i = -1; auto val : arr)
		{
			if (++i >= 3) break;
			int64_t value;
			if (!val.get_int64().get(value)) out[i] = TO_INT(value);
		}
	}
}

static void ParseObject(simdjson::ondemand::object& doc, sObject3d parent, sObject3d scene, void* physics, int depth)
{
	// 1) default values
	std::array<int, 3> irot     = {};
	std::string        name     = "";
	glm::vec3          position = glm::vec3(0.0f);
	glm::vec3          scale    = glm::vec3(1.0f);
	int                type     = ObjectType_Basic;
	bool               visible  = true;

	// temp vars
	simdjson::ondemand::array  array      = {};
	simdjson::ondemand::object obj        = {};
	bool                       tempBool   = true;
	int64_t                    tempInt64  = 0;
	std::string                tempString = "";

	// 2) name / type / visible
	if (!doc["name"].get_string().get(name)) {}
	if (!doc["type"].get_string().get(tempString)) type = ObjectType(tempString);
	if (!doc["visible"].get_bool().get(tempBool)) visible = tempBool;

	// 3) irot / position / scale
	GetArrayInt(doc, "irot", irot);
	GetArrayFloat(doc, "position", glm::value_ptr(position));
	GetArrayFloat(doc, "scale", glm::value_ptr(scale));

	// 4) create object based on type
	sObject3d exist      = nullptr;
	sObject3d object     = nullptr;
	bool      positioned = false;

	if (type & ObjectType_Camera)
	{
		exist = scene->GetObjectByName("Camera");
		if (exist) object = exist;

		if (auto camera = Camera::SharedPtr(object))
		{
			GetArrayFloat(doc, "pos", &camera->pos.x);
			GetArrayFloat(doc, "target", &camera->target.x);

			std::memcpy(&camera->pos2.x, &camera->pos.x, sizeof(bx::Vec3));
			std::memcpy(&camera->target2.x, &camera->target.x, sizeof(bx::Vec3));
		}
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
	else if (type & ObjectType_RubikCube)
	{
		int cubeSize = 3;
		if (!doc["cubicSize"].get_int64().get(tempInt64)) cubeSize = TO_INT(tempInt64);
		object = std::make_shared<RubikCube>(name, cubeSize);
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
		sMesh mesh = Mesh::SharedPtr(object);

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
			{
				if (!mesh->material)
					mesh->material = std::make_shared<Material>(vsName, fsName);

				// state
				{
					int64_t state;
					if (!doc["state"].get_int64().get(state))
						mesh->material->state = state;
				}

				// textures
				if (!obj["texNames"].get_array().get(array))
				{
					VEC_STR texFiles;
					for (int i = -1; std::string_view texName : array)
						texFiles.push_back(std::string(texName));

					mesh->material->LoadTextures(texFiles);
				}
			}
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
		ui::Log(" -->4 type={}/{} {}", type, object->type, name);
	}
	else if (type & ObjectType_Scene)
	{
		exist = scene;
		if (exist) object = exist;
	}
	else object = std::make_shared<Object3d>(name, type);

	// 5) set properties
	if (object)
	{
		object->visible = visible;
		if (!positioned) object->ScaleIrotPosition(scale, irot, position);
	}

	// 6) connect node
	if (DEV_scene) ui::Log("ParseObject: {} {}/{} {}", depth, type, object->type, object->name);
	if (object && !exist)
		parent->AddChild(object);

	// 7) check children
	if (!doc["children"].get_array().get(array))
	{
		for (simdjson::ondemand::object child : array)
			ParseObject(child, object ? object : parent, scene, physics, depth + 1);
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
		Scene::SharedPtr(scene)->Clear();
		ParseObject(obj, scene, scene, physics.get(), 0);
		return true;
	}

	return false;
}

bool App::SaveScene(const std::filesystem::path& filename)
{
	std::filesystem::path output = filename.empty() ? xsettings.recentFiles[0] : filename;

	fmt::memory_buffer outString;
	scene->Serialize(outString, 0);
	ui::Log("SaveScene: {}", OUTSTRING_VIEW);
	WriteData(output, OUTSTRING_VIEW);
	AddRecent(output);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCENE
////////

void Scene::Clear()
{
	for (auto& child : children)
	{
		// empty the map but don't delete the Map itself
		if (child->type & ObjectType_Map)
			child->ClearDeads(true);
		// delete everything except Camera/Cursor
		else if (!(child->type & (ObjectType_Camera | ObjectType_Cursor)))
			child->dead |= Dead_Remove;
	}

	ClearDeads(false);
}

void App::PickObject(int mouseX, int mouseY)
{
	ui::Log("PickObject: {} {}", mouseX, mouseY);
}

void App::SelectObject(const sObject3d& obj, bool countIndex)
{
	// 1) place + restore material
	if (const auto& temp = selectWeak.lock())
	{
		temp->placed = true;

		if (auto mesh = Mesh::SharedPtr(temp); mesh && mesh->material0)
			mesh->material = mesh->material0;
	}

	// 2) new selection
	if (!obj)
	{
		camera->follow  = CameraFollow_Cursor;
		cursor->visible = true;
	}
	else
	{
		prevSelWeak = selectWeak;
		selectWeak  = obj;

		camera->follow |= CameraFollow_Active | CameraFollow_SelectedObj;
		camera->follow &= ~CameraFollow_Cursor;

		if (auto mesh = Mesh::SharedPtr(obj))
		{
			mesh->material0 = mesh->material;
			mesh->material  = GetMaterialManager().GetMaterial("cursor");
			cursor->visible = false;
		}

		// find the parent's childId
		if (countIndex)
		{
			if (auto* parent = obj->parent)
			{
				for (int id = -1; const auto& child : parent->children)
				{
					++id;
					if (child == obj)
					{
						parent->childId = id;
						break;
					}
				}
			}
		}
	}

	// 3) camera on target
	if (const sObject3d target = obj ? obj : cursor)
	{
		camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(target->position));
		camera->Zoom();

		MoveSelected(true);
		if (DEV_matrix) PrintMatrix(target->matrixWorld, target->name);
		FocusScreen();
	}
}
