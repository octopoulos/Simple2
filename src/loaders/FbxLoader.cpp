// FbxLoader.cpp
// @author octopoulos
// @version 2025-09-18

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include <ofbx.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

/// Create material from FBX
static sMaterial CreateMaterialFromFbx(const ofbx::IScene& scene, const ofbx::Material* fbxMaterial, const std::filesystem::path& fbxPath)
{
	std::string      vsName       = "vs_model_color"; // default to color-based lit shader
	std::string      fsName       = "fs_model_color";
	sMaterial        material     = nullptr;
	std::string      materialName = "DefaultMaterial";
	int              numTexture   = 0;
	VEC<TextureData> textures     = {};

	if (fbxMaterial)
	{
		materialName = fbxMaterial->name[0] ? std::string(fbxMaterial->name) : Format("Material_%d", fbxMaterial->id);

		// process textures
		for (int type = 0; type < TextureType_Count; ++type)
		{
			if (const ofbx::Texture* texture = fbxMaterial->getTexture(ofbx::Texture::TextureType(type)))
			{
				const auto typeName = TextureName(type);

				if (type == ofbx::Texture::DIFFUSE)
				{
					vsName = "vs_model_texture_normal";
					fsName = "fs_model_texture_normal";
				}

				ofbx::DataView filename = texture->getFileName();
				char           texturePath[256];
				filename.toString(texturePath);
				ui::Log("TEX: {}={} for {}", typeName, texturePath, materialName);

				TextureData texData = { type, "" };
				if (texturePath[0])
				{
					texData.name        = std::filesystem::path(texturePath).filename().string();
					const auto fullPath = fbxPath.parent_path() / texData.name;
					texData.handle      = GetTextureManager().LoadTexture(fullPath.string());
					if (bgfx::isValid(texData.handle))
						ui::Log("CreateMaterialFromFbx: loaded {} texture {} for {}", typeName, texData.name, materialName);
					else
						ui::LogError("CreateMaterialFromFbx: failed {} file: {} for {}", typeName, fullPath.string(), materialName);
					textures.push_back(texData);
				}

				// check for embedded texture data
				if (texData.name.empty())
				{
					ofbx::DataView embeddedData = texture->getEmbeddedData();
					if (embeddedData.begin && embeddedData.end)
					{
						texData.name   = Format("embedded_%d_%s", fbxMaterial->id, Cstr(typeName));
						texData.handle = GetTextureManager().AddRawTexture(texData.name, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
						if (bgfx::isValid(texData.handle))
							ui::Log("CreateMaterialFromFbx: loaded embedded {} texture {} for {}", typeName, texData.name, materialName);
						else
							ui::LogError("CreateMaterialFromFbx: failed embedded {} for {}", typeName, materialName);
						textures.push_back(texData);
					}
				}
			}
		}

		// load material with texture names
		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, {}, textures);

		// PBR-like properties
		ofbx::Color diffuse   = fbxMaterial->getDiffuseColor();
		ofbx::Color emissive  = fbxMaterial->getEmissiveColor();
		float       metallic  = 0.0f;
		float       roughness = 1.0f;
		material->SetPbrProperties(
		    glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f),
		    metallic,
		    roughness);
		material->emissiveFactor = glm::vec3(emissive.r, emissive.g, emissive.b);

		// alpha and rendering state
		int alphaMode = AlphaMode_Opaque;
		if (const float shininess = fbxMaterial->getShininess(); shininess < 1.0f)
			alphaMode = AlphaMode_Blend;
		material->alphaCutoff = 0.5f;
		material->alphaMode   = alphaMode;
		material->doubleSided = false;
		material->unlit       = false;

		uint64_t state = BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_MSAA | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z;
		if (alphaMode == AlphaMode_Blend) state |= BGFX_STATE_BLEND_ALPHA;
		if (!material->doubleSided) state |= BGFX_STATE_CULL_CCW;
		material->state = state;
	}
	else
	{
		material = GetMaterialManager().LoadMaterial(materialName, "vs_model_color", "fs_model_color");
		material->SetPbrProperties(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.0f, 1.0f);
	}

	return material;
}

/// Create mesh for FBX node
static sMesh CreateNodeMesh(const ofbx::Object* node)
{
	const char* nodeName = node->name[0] ? node->name : Format("Node_%d", node->id);
	sMesh       mesh     = std::make_shared<Mesh>(nodeName, 0);
	mesh->type |= ObjectType_Mesh;

	// set local transform
	auto      fbxMatrix = node->getLocalTransform();
	glm::mat4 glmMatrix;
	for (int i = 0; i < 16; ++i)
		glmMatrix[i / 4][i % 4] = TO_FLOAT(fbxMatrix.m[i]);

	mesh->matrix = glmMatrix;
	mesh->DecomposeMatrix();
	mesh->UpdateLocalMatrix("CreateNodeMesh");

	// clang-format off
	mesh->aabb   = { { FLT_MAX, FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
	mesh->sphere = { { 0.0f, 0.0f, 0.0f }, 0.0f };
	// clang-format on

	ui::Log("CreateNodeMesh: {}", nodeName);
	return mesh;
}

/// Process FBX node recursively
static sMesh ProcessMesh(const ofbx::IScene& scene, const ofbx::Mesh* fbxMesh, const std::filesystem::path& fbxPath)
{
	// 1) create mesh
	sMesh      mesh     = CreateNodeMesh(fbxMesh);
	const auto nodeName = mesh->name;

	ui::Log("ProcessMesh: {}", nodeName);

	// 2) process geometry
	const ofbx::GeometryData&  geom      = fbxMesh->getGeometryData();
	const ofbx::Vec3Attributes positions = geom.getPositions();
	const ofbx::Vec3Attributes normals   = geom.getNormals();
	const ofbx::Vec2Attributes uvs       = geom.getUVs();
	const ofbx::Vec4Attributes colors    = geom.getColors();

	// 3) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float, true)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Float)
		.end();
	// clang-format on
	mesh->layout = layout;

	// 4) vertex struct
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 uv;
		glm::vec4 color;

		Vertex()
		    : position(0.0f)
		    , normal(0.0f, 0.0f, 1.0f)
		    , uv(0.0f)
		    , color(1.0f)
		{
		}
	};

	// 5) process partitions (materials)
	for (int partitionId = 0; partitionId < geom.getPartitionCount(); ++partitionId)
	{
		Group       group;
		const auto& partition = geom.getPartition(partitionId);

		// material
		const auto* material = fbxMesh->getMaterial(partitionId);
		group.material       = CreateMaterialFromFbx(scene, material, fbxPath);
		if (!group.material)
		{
			ui::LogError("ProcessMesh: Cannot create material for mesh {} partition {}", nodeName, partitionId);
			continue;
		}

		// vertices and indices + positions (for AABB)
		std::vector<uint32_t> indices;
		std::vector<Vertex>   vertices;
		std::vector<bx::Vec3> vpositions;

		// clang-format off
		bx::Aabb groupAabb = { { FLT_MAX, FLT_MAX, FLT_MAX  }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
		bx::Sphere groupSphere = { { 0.0f, 0.0f, 0.0f }, 0.0f };
		// clang-format on

		// process polygons
		for (int polygonId = 0; polygonId < partition.polygon_count; ++polygonId)
		{
			const auto& polygon = partition.polygons[polygonId];
			int         triIndices[128];
			const int   triCount = ofbx::triangulate(geom, polygon, triIndices);
			// ui::Log("Mesh {} partition {} polygon {}: {} indices", nodeName, partitionId, polygonId, triCount);
			if (triCount % 3 != 0)
			{
				ui::LogError("ProcessMesh: Invalid triangulation for mesh {} partition {} polygon {}: {} indices", nodeName, partitionId, polygonId, triCount);
				continue;
			}

			for (int i = 0; i < triCount; ++i)
			{
				int vertexId = triIndices[i];
				if (vertexId < 0 || vertexId >= positions.count)
				{
					ui::LogError("ProcessMesh: Invalid vertex index {} for mesh {} partition {} polygon {}", vertexId, nodeName, partitionId, polygonId);
					continue;
				}

				Vertex     vertex;
				ofbx::Vec3 pos  = positions.get(vertexId);
				vertex.position = glm::vec3(TO_FLOAT(pos.x), TO_FLOAT(pos.y), TO_FLOAT(pos.z));
				vpositions.push_back({ vertex.position.x, vertex.position.y, vertex.position.z });

				if (normals.values && vertexId < normals.count)
				{
					ofbx::Vec3 norm = normals.get(vertexId);
					vertex.normal   = glm::vec3(TO_FLOAT(norm.x), TO_FLOAT(norm.y), TO_FLOAT(norm.z));
				}
				if (uvs.values && vertexId < uvs.count)
				{
					ofbx::Vec2 uv = uvs.get(vertexId);
					vertex.uv     = glm::vec2(TO_FLOAT(uv.x), TO_FLOAT(uv.y));
				}
				if (colors.values && vertexId < colors.count)
				{
					ofbx::Vec4 col = colors.get(vertexId);
					vertex.color   = glm::vec4(TO_FLOAT(col.x), TO_FLOAT(col.y), TO_FLOAT(col.z), TO_FLOAT(col.w));
				}

				vertices.push_back(vertex);
				indices.push_back(TO_UINT32(vertices.size() - 1));
			}
		}

		// calculate group AABB and sphere
		const uint32_t numVpos = TO_UINT32(vpositions.size());
		if (numVpos)
		{
			bx::toAabb(groupAabb, vpositions.data(), numVpos, sizeof(bx::Vec3));
			bx::calcMaxBoundingSphere(groupSphere, vpositions.data(), numVpos, sizeof(bx::Vec3));
		}
		else
		{
			ui::LogError("ProcessMesh: No vertices for mesh {} partition {}", nodeName, partitionId);
			continue;
		}

		// update groups AABB and sphere
		group.aabb   = groupAabb;
		group.sphere = groupSphere;

		// update mesh AABB and sphere
		mesh->aabb.min = bx::min(mesh->aabb.min, groupAabb.min);
		mesh->aabb.max = bx::max(mesh->aabb.max, groupAabb.max);
		bx::Sphere tempSphere;
		bx::calcMaxBoundingSphere(tempSphere, vpositions.data(), numVpos, sizeof(bx::Vec3));
		if (tempSphere.radius > mesh->sphere.radius)
			mesh->sphere = tempSphere;

		// create BGFX buffers
		if (!vertices.empty())
		{
			group.vertices = static_cast<uint8_t*>(bx::alloc(entry::getAllocator(), vertices.size() * sizeof(Vertex)));
			memcpy(group.vertices, vertices.data(), vertices.size() * sizeof(Vertex));
			group.numVertices = TO_UINT32(vertices.size());
			group.vbh         = bgfx::createVertexBuffer(bgfx::makeRef(group.vertices, vertices.size() * sizeof(Vertex)), layout, BGFX_BUFFER_NONE);
			ui::Log("ProcessMesh: mesh {} partition {}: {} vertices", nodeName, partitionId, vertices.size());
		}
		else
		{
			ui::LogError("ProcessMesh: No vertices for mesh {} partition {}", nodeName, partitionId);
			continue;
		}

		if (!indices.empty())
		{
			group.indices = static_cast<uint32_t*>(bx::alloc(entry::getAllocator(), indices.size() * sizeof(uint32_t)));
			memcpy(group.indices, indices.data(), indices.size() * sizeof(uint32_t));
			group.numIndices = TO_UINT32(indices.size());
			group.ibh        = bgfx::createIndexBuffer(bgfx::makeRef(group.indices, indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
			ui::Log("ProcessMesh: mesh {} partition {}: {} indices", nodeName, partitionId, indices.size());
		}
		else
		{
			ui::LogError("ProcessMesh: No indices for mesh {} partition {}", nodeName, partitionId);
			continue;
		}

		// set up primitive
		Primitive prim;
		prim.startIndex  = 0;
		prim.numIndices  = group.numIndices;
		prim.startVertex = 0;
		prim.numVertices = group.numVertices;
		prim.aabb        = groupAabb;
		prim.sphere      = groupSphere;
		group.prims.push_back(prim);

		mesh->groups.push_back(std::move(group));
	}

	// 6) set default material for the mesh
	mesh->material  = CreateMaterialFromFbx(scene, nullptr, fbxPath);
	mesh->material0 = mesh->material;

	return mesh;
}

static bx::Aabb TransformAabb(const bx::Aabb& aabb, const glm::mat4& matrix)
{
	bx::Aabb transformedAabb = { { FLT_MAX, FLT_MAX, FLT_MAX  }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
	if (aabb.min.x >= aabb.max.x)
	{
		ui::LogError("TransformAabb: Invalid input AABB");
		return transformedAabb;
	}

	// get the eight corners of the AABB
	bx::Vec3 corners[8] = {
		{ aabb.min.x, aabb.min.y, aabb.min.z },
		{ aabb.max.x, aabb.min.y, aabb.min.z },
		{ aabb.min.x, aabb.max.y, aabb.min.z },
		{ aabb.max.x, aabb.max.y, aabb.min.z },
		{ aabb.min.x, aabb.min.y, aabb.max.z },
		{ aabb.max.x, aabb.min.y, aabb.max.z },
		{ aabb.min.x, aabb.max.y, aabb.max.z },
		{ aabb.max.x, aabb.max.y, aabb.max.z },
	};

	// Transform corners by matrix
	for (int j = 0; j < 8; ++j)
	{
		glm::vec4 corner = matrix * glm::vec4(corners[j].x, corners[j].y, corners[j].z, 1.0f);
		corners[j]       = { corner.x, corner.y, corner.z };
	}

	// Compute AABB from transformed corners
	bx::toAabb(transformedAabb, corners, 8, sizeof(bx::Vec3));
	return transformedAabb;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

sMesh LoadFbx(const std::filesystem::path& path, bool ramcopy)
{
	if (!std::filesystem::exists(path))
	{
		ui::LogError("LoadFbx: File not found {}", path.string());
		return nullptr;
	}

	ui::Log("LoadFbx: {}", path.string());

	// load FBX file in binary
	std::string content = ReadData(path);
	if (content.empty())
	{
		ui::LogError("LoadFbx: Failed to read file {}", path.string());
		return nullptr;
	}

	// load flags
	ofbx::LoadFlags flags = ofbx::LoadFlags::NONE
	    | ofbx::LoadFlags::IGNORE_ANIMATIONS
	    | ofbx::LoadFlags::IGNORE_BLEND_SHAPES
	    | ofbx::LoadFlags::IGNORE_BONES
	    | ofbx::LoadFlags::IGNORE_CAMERAS
	    | ofbx::LoadFlags::IGNORE_LIGHTS
	    | ofbx::LoadFlags::IGNORE_LIMBS
	    | ofbx::LoadFlags::IGNORE_PIVOTS
	    | ofbx::LoadFlags::IGNORE_POSES
	    | ofbx::LoadFlags::IGNORE_SKIN
	    | ofbx::LoadFlags::IGNORE_VIDEOS;

	ofbx::IScene* scene = ofbx::load(reinterpret_cast<const ofbx::u8*>(content.data()), content.size(), static_cast<ofbx::u16>(flags));
	if (!scene)
	{
		ui::LogError("LoadFbx: Failed to load scene {}: {}", path, ofbx::getError());
		return nullptr;
	}

	// create root mesh
	sMesh rootMesh = std::make_shared<Mesh>(path.filename().string(), ObjectType_Group);

	// map to track meshes by object id
	std::unordered_map<uint64_t, sMesh> meshMap;
	meshMap[rootMesh->id] = rootMesh;

	// process all meshes
	for (int i = 0, meshCount = scene->getMeshCount(); i < meshCount; ++i)
	{
		const ofbx::Mesh* fbxMesh = scene->getMesh(i);
		sMesh             mesh    = ProcessMesh(*scene, fbxMesh, path);
		meshMap[fbxMesh->id]      = mesh;

		ui::Log("LoadFbx: {}/{} name={}", i, meshCount, mesh->name);

		// find parent mesh
		sMesh               parentMesh = rootMesh;
		const ofbx::Object* parent     = fbxMesh->getParent();
		if (parent)
		{
			if (auto it = meshMap.find(parent->id); it != meshMap.end())
				parentMesh = it->second;
			else
			{
				// create parent node (aabb initialized in CreateNodeMesh)
				parentMesh          = CreateNodeMesh(parent);
				meshMap[parent->id] = parentMesh;

				// add to hierarchy
				sMesh               grandParentMesh = rootMesh;
				const ofbx::Object* grandParent     = parent->getParent();
				if (grandParent)
				{
					if (auto grandIt = meshMap.find(grandParent->id); grandIt != meshMap.end())
						grandParentMesh = grandIt->second;
				}
				grandParentMesh->AddChild(parentMesh);
				ui::Log("LoadFbx: parent node {} for mesh {}", parentMesh->name, mesh->name);
			}
		}

		// add mesh to parent
		parentMesh->AddChild(mesh);
		ui::Log("LoadFbx: add mesh {} to parent {}", mesh->name, parentMesh->name);

		// update parent AABBs up the hierarchy
		sMesh current = mesh;
		while (auto parent = current ? Mesh::SharedPtr(current->parent.lock()) : nullptr)
		{
			if (current->aabb.min.x >= current->aabb.max.x)
			{
				ui::LogError("LoadFbx: Invalid AABB for mesh {}", current->name);
				break;
			}

			// transform child AABB to parent space
			bx::Aabb transformedAabb = TransformAabb(current->aabb, current->matrix);

			// aggregate into parent AABB
			parent->aabb.min = bx::min(parent->aabb.min, transformedAabb.min);
			parent->aabb.max = bx::max(parent->aabb.max, transformedAabb.max);

			// update groups AABB for group-type parent
			if (parent->type & ObjectType_Group)
				for (auto& group : parent->groups)
					group.aabb = parent->aabb;

			ui::Log("LoadFbx: updated parent {}: AABB min=({:.2f}, {:.2f}, {:.2f}), max=({:.2f}, {:.2f}, {:.2f})", parent->name, parent->aabb.min.x, parent->aabb.min.y, parent->aabb.min.z, parent->aabb.max.x, parent->aabb.max.y, parent->aabb.max.z);

			current = parent;
		}
	}

	// log hierarchy
	for (const auto& child : rootMesh->children)
	{
		const auto sparent = child->parent.lock();
		ui::Log("LoadFbx: child {}: parent={} groups={} children={}", child->name, sparent ? sparent->name : "none", Mesh::SharedPtr(child)->groups.size(), child->children.size());
	}

	// clean up
	scene->destroy();

	// if it's a scene with one child => return that child
	return (rootMesh->children.size() == 1) ? Mesh::SharedPtr(rootMesh->children[0]) : rootMesh;
}
