// Scene.cpp
// @author octopoulos
// @version 2025-09-27

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

#include "BulletCollision/NarrowPhaseCollision/btRaycastCallback.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// APP
//////

/// Add to recent files
/// - this means a file was just loaded/saved
static void AddRecent(const std::filesystem::path& filename)
{
	str2k temp;
	strcpy(temp, Cstr(filename));

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

static bool GetArrayFloat(int count, simdjson::ondemand::object& doc, const char* key, float* out, const std::vector<float> reset = {})
{
	if (reset.size()) memcpy(out, reset.data(), count * sizeof(float));

	simdjson::ondemand::array arr;
	if (!doc[key].get_array().get(arr))
	{
		for (int i = -1; auto val : arr)
		{
			if (++i >= count) break;
			double value;
			if (!val.get_double().get(value)) out[i] = TO_FLOAT(value);
		}
		return true;
	}
	return false;
}

static bool GetArrayInt(int count, simdjson::ondemand::object& doc, const char* key, int* out, const std::vector<int> reset = {})
{
	if (reset.size()) memcpy(out, reset.data(), count * sizeof(int));

	simdjson::ondemand::array arr;
	if (!doc[key].get_array().get(arr))
	{
		for (int i = -1; auto val : arr)
		{
			if (++i >= count) break;
			int64_t value;
			if (!val.get_int64().get(value)) out[i] = TO_INT(value);
		}
		return true;
	}
	return false;
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
	simdjson::ondemand::array  array         = {};
	simdjson::ondemand::object obj           = {};
	bool                       tempBool      = true;
	float                      tempFloat4[4] = {};
	int64_t                    tempInt64     = 0;
	std::string                tempString    = "";

	// 2) name / type / visible
	if (!doc["name"].get_string().get(name)) {}
	if (!doc["type"].get_string().get(tempString)) type = ObjectType(tempString);
	if (!doc["visible"].get_bool().get(tempBool)) visible = tempBool;

	// 3) irot / position / scale
	// clang-format off
	GetArrayInt  (3, doc, "irot"    , irot.data()             , { 0, 0, 0 });
	GetArrayFloat(3, doc, "position", glm::value_ptr(position), { 0.0f, 0.0f, 0.0f });
	GetArrayFloat(3, doc, "scale"   , glm::value_ptr(scale)   , { 1.0f, 1.0f, 1.0f });
	// clang-format on

	// 4) create object based on type
	sObject3d  exist      = nullptr;
	sObject3d  object     = nullptr;
	bool       positioned = false;
	sRubikCube rubik      = nullptr;

	if (type & ObjectType_Camera)
	{
		exist = scene->GetObjectByName("Camera");
		if (exist) object = exist;

		if (auto camera = Camera::SharedPtr(object))
		{
			// clang-format off
			GetArrayFloat(3, doc, "pos"   , &camera->pos.x   , { 0.0f, 0.0f, 0.0f });
			GetArrayFloat(3, doc, "target", &camera->target.x, { 0.0f, 0.0f, 1.0f });
			// clang-format on

			memcpy(&camera->pos2.x, &camera->pos.x, sizeof(bx::Vec3));
			memcpy(&camera->target2.x, &camera->target.x, sizeof(bx::Vec3));
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
	else if (type & ObjectType_Mesh)
	{
		std::string texPath;
		if (!doc["texPath"].get_string().get(texPath)) {}

		// rubik
		if (type & ObjectType_RubikCube)
		{
			int cubeSize = 3;
			if (!doc["cubicSize"].get_int64().get(tempInt64)) cubeSize = TO_INT(tempInt64);
			object = std::make_shared<RubikCube>(name, cubeSize);
			rubik  = RubikCube::SharedPtr(object);
		}
		else if (int64_t load = 0; !doc["load"].get_int64().get(load))
		{
			std::string modelName;
			if (!doc["modelName"].get_string().get(modelName))
				object = MeshLoader::LoadModelFull(name, modelName, {}, texPath);
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
					mesh->material = std::make_shared<Material>(Cstr(vsName), Cstr(fsName));

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
			btVector4 dims      = { 0.0f, 0.0f, 0.0f, 0.0f };
			bool      enabled   = true;
			double    mass      = 0.0;
			int       shapeType = ShapeType_Box;

			if (!doc["body"].get_object().get(obj))
			{
				GetArrayFloat(4, obj, "dims", tempFloat4, { 0.0f, 0.0f, 0.0f, 0.0f });
				dims = btVector4(tempFloat4[0], tempFloat4[1], tempFloat4[2], tempFloat4[3]);

				if (!obj["enabled"].get_bool().get(tempBool)) enabled = tempBool;
				if (!obj["mass"].get_double().get(mass)) {}
				if (!obj["shapeType"].get_string().get(tempString)) shapeType = ShapeType(tempString);

				ui::Log("XX name={} type={}", name, type);
				ui::Log("XX dims={} {} {} {} => {} {} {} {}", tempFloat4[0], tempFloat4[1], tempFloat4[2], tempFloat4[3], dims.x(), dims.y(), dims.z(), dims.w());
				ui::Log("XX mass={}", mass);
				ui::Log("XX shapeType={} ({})", shapeType, tempString);
			}

			mesh->ScaleIrotPosition(scale, irot, position);
			positioned = true;
			mesh->CreateShapeBody((PhysicsWorld*)physics, shapeType, TO_FLOAT(mass), dims);
			mesh->body->enabled = enabled;
		}
		ui::Log(" -->4 type={}/{} {}", type, object->type, name);

		// extras
		// if (type & ObjectType_RubikCube)
		// {
		// 	RubikCube::SharedPtr(mesh)->Initialize();
		// 	mesh->SetPhysics((PhysicsWorld*)physics);
		// }
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

	// 7) extra initializers
	if (rubik)
	{
		rubik->Initialize();
		rubik->SetPhysics((PhysicsWorld*)physics);
	}

	// 8) check children
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
		ParseObject(obj, scene, scene, GetPhysics(), 0);

		entry::setWindowTitle(entry::kDefaultWindowHandle, Cstr(filename.filename()));
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
	if (!xsettings.picking) return;
	ui::Log("PickObject: Mouse ({}, {})", mouseX, mouseY);

	// Get screen dimensions from xsettings
	const float screenX = static_cast<float>(xsettings.windowSize[0]);
	const float screenY = static_cast<float>(xsettings.windowSize[1]);
	if (screenX <= 0 || screenY <= 0)
	{
		ui::LogError("PickObject: Invalid window size ({}, {})", screenX, screenY);
		return;
	}

	// 1) Convert mouse coords to normalized device coordinates (NDC: [-1, 1])
	const float ndcX = (2.0f * mouseX) / screenX - 1.0f;
	const float ndcY = 1.0f - (2.0f * mouseY) / screenY; // Flip Y (screen Y=0 is top)
	ui::Log("PickObject: NDC ({:.2f}, {:.2f})", ndcX, ndcY);

	// 2) Get view and projection matrices from camera
	float view[16];
	camera->GetViewMatrix(view);
	glm::mat4 viewMtx = glm::make_mat4(view);

	float      proj[16];
	const bool homoDepth = bgfx::getCaps()->homogeneousDepth;
	if (xsettings.projection == Projection_Orthographic)
	{
		const float zoomX = screenX * xsettings.orthoZoom;
		const float zoomY = screenY * xsettings.orthoZoom;
		bx::mtxOrtho(proj, -zoomX, zoomX, -zoomY, zoomY, -1000.0f, 1000.0f, 0.0f, homoDepth);
	}
	else
	{
		bx::mtxProj(proj, xsettings.fov, screenX / screenY, 0.1f, 2000.0f, homoDepth);
	}
	glm::mat4 projMtx = glm::make_mat4(proj);

	// 3) Compute inverse view-projection matrix
	glm::mat4 invViewProj = glm::inverse(projMtx * viewMtx);

	// 4) Ray origin and direction in world space
	glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
	glm::vec4 farPoint  = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
	nearPoint /= nearPoint.w;
	farPoint /= farPoint.w;

	glm::vec3 rayOrigin = glm::vec3(nearPoint);
	glm::vec3 rayDir    = glm::normalize(glm::vec3(farPoint - nearPoint));
	ui::Log("PickObject: Ray from ({:.2f}, {:.2f}, {:.2f}) dir ({:.2f}, {:.2f}, {:.2f})", rayOrigin.x, rayOrigin.y, rayOrigin.z, rayDir.x, rayDir.y, rayDir.z);

	// 5) Perform Bullet raycast
	btVector3 from  = GlmToBullet(rayOrigin);
	btVector3 to    = GlmToBullet(rayOrigin + rayDir * xsettings.rayLength); // Configurable distance
	auto*     world = GetPhysics()->GetWorld();
	ui::Log("PickObject: Physics world has {} objects", world->getNumCollisionObjects());

	// Update AABBs and pairs (like RaytestDemo)
	world->updateAabbs();
	world->computeOverlappingPairs();

	btCollisionWorld::ClosestRayResultCallback rayCallback(from, to);
	rayCallback.m_flags |= btTriangleRaycastCallback::kF_FilterBackfaces;                // Filter backfaces
	rayCallback.m_flags |= btTriangleRaycastCallback::kF_UseSubSimplexConvexCastRaytest; // More accurate raycast
	world->rayTest(from, to, rayCallback);

	// Visualize ray (red if no hit, green if hit)
	btVector3 color = rayCallback.hasHit() ? btVector3(0, 1, 0) : btVector3(1, 0, 0);
	world->getDebugDrawer()->drawLine(from, to, color);
	if (rayCallback.hasHit())
	{
		btVector3 hitPoint = from.lerp(to, rayCallback.m_closestHitFraction);
		world->getDebugDrawer()->drawSphere(hitPoint, 0.1f, btVector3(0, 1, 0));
		world->getDebugDrawer()->drawLine(hitPoint, hitPoint + rayCallback.m_hitNormalWorld, btVector3(0, 1, 0));
	}

	// 6) Process hit
	if (rayCallback.hasHit())
	{
		const btCollisionObject* hitObject = rayCallback.m_collisionObject;
		ui::Log("PickObject: Hit at ({:.2f}, {:.2f}, {:.2f})", rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());

		if (void* userPtr = hitObject->getUserPointer())
		{
			Mesh* hitMesh = static_cast<Mesh*>(userPtr);
			ui::Log("PickObject: userPointer = {}, type = {}, name = '{}'", (void*)hitMesh, hitMesh->type, hitMesh->name);

			if (hitMesh && (hitMesh->type & ObjectType_Mesh))
			{
				try
				{
					sObject3d hitObj = hitMesh->Object3d::shared_from_this();
					SelectObject(hitObj);
					camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(hitMesh->position));
					camera->Zoom();
					ui::Log("PickObject: Selected mesh '{}' at position ({:.2f}, {:.2f}, {:.2f})", hitMesh->name, hitMesh->position.x, hitMesh->position.y, hitMesh->position.z);
					ui::Log("PickObject: Camera target2 = ({:.2f}, {:.2f}, {:.2f})", camera->target2.x, camera->target2.y, camera->target2.z);
					return;
				}
				catch (const std::bad_weak_ptr& e)
				{
					ui::LogError("PickObject: shared_from_this failed for mesh '{}'", hitMesh->name);
				}
			}
			else
			{
				ui::Log("PickObject: Invalid mesh or type");
			}
		}
		else
		{
			ui::Log("PickObject: No userPointer set");
		}
	}
	else
	{
		ui::Log("PickObject: No hit");
	}

	// No valid hit, deselect
	SelectObject(nullptr);
}

void App::SelectObject(const sObject3d& obj, bool countIndex)
{
	// 1) place + restore material
	if (const auto temp = selectWeak.lock())
	{
		temp->placing = false;

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
			std::string_view shaderName = ((mesh->type & ObjectType_Group) && (mesh->type & ObjectType_Instance)) ? "cursor_instance" : "cursor";

			mesh->material0 = mesh->material;
			mesh->material  = GetMaterialManager().GetMaterial(shaderName);
			cursor->visible = false;
		}

		// find the parent's childId
		if (countIndex)
		{
			if (auto parent = obj->parent.lock())
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
