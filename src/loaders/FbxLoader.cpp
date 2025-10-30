// FbxLoader.cpp
// @author octopoulos
// @version 2025-10-25

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "core/common3d.h"             // ComputeTangentsMikktspace, Vertex
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include <ofbx.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

/// Converts an FBX (Blender) matrix to engine-space matrix
/// Blender: X=Right, Y=Forward, Z=Up (Left-handed)
/// Engine:  X=Right, Y=Up, Z=Forward (Right-handed)
static glm::mat4 FbxToMatrix(const ofbx::DMatrix& fbxMat)
{
	glm::mat4 matrix = glm::make_mat4(fbxMat.m);

	static const glm::mat4 coord = glm::mat4(
		-1.0f, 0.0f, 0.0f, 0.0f,  // X stays X (right)
		0.0f, 0.0f, 1.0f, 0.0f,  // Z -> Y (up)
		0.0f, -1.0f, 0.0f, 0.0f, // Y -> -Z (forward)
		0.0f, 0.0f, 0.0f, 1.0f
	);

	return matrix * coord;
}

/// Y -> -Z, Z -> Y
inline glm::vec3 FbxToNormal(const ofbx::Vec3& n) { return glm::normalize(glm::vec3(TO_FLOAT(-n.x), TO_FLOAT(n.z), TO_FLOAT(-n.y))); }

/// Y -> -Z, Z -> Y
inline glm::vec3 FbxToPosition(const ofbx::Vec3& v) { return glm::vec3(TO_FLOAT(-v.x), TO_FLOAT(v.z), TO_FLOAT(-v.y)); }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

/// Create material from FBX
static sMaterial CreateMaterialFromFbx(const ofbx::IScene& scene, const ofbx::Material* fbxMaterial, const std::filesystem::path& fbxPath, std::string_view texPath)
{
	const auto       baseName     = fbxPath.stem().string();
	std::string      fsName       = "fs_pbr";
	std::string      vsName       = "vs_pbr";
	sMaterial        material     = nullptr;
	std::string      materialName = "DefaultMaterial";
	int              numTexture   = 0;
	VEC<TextureData> textures     = {};

	if (fbxMaterial)
	{
		materialName = fbxMaterial->name[0] ? FormatStr("%s:%s", Cstr(baseName), fbxMaterial->name) : FormatStr("%s:#%d", Cstr(baseName), fbxMaterial->id);

		// process textures
		bool hasPbrTextures = false;
		for (int type = 0; type < TextureType_Count; ++type)
		{
			if (const ofbx::Texture* texture = fbxMaterial->getTexture(ofbx::Texture::TextureType(type)))
			{
				const auto typeName = TextureName(type);

				if (type == ofbx::Texture::DIFFUSE || type == ofbx::Texture::NORMAL || type == ofbx::Texture::SPECULAR)
				{
					vsName         = "vs_pbr";
					fsName         = "fs_pbr";
					hasPbrTextures = true;
				}

				ofbx::DataView filename = texture->getFileName();
				char           textureCpath[256];
				filename.toString(textureCpath);
				ui::Log("TEX: %s=%s for %s", Cstr(typeName), textureCpath, Cstr(materialName));

				TextureData texData = { type, "" };
				if (textureCpath[0])
				{
					auto       tryPath  = std::filesystem::path(NormalizeFilename(textureCpath));
					const auto filename = tryPath.filename().string();
					texData.name        = filename;

					if (!IsFile(tryPath))
					{
						if (texPath.size())
							tryPath = std::filesystem::path(texPath) / filename;
						else
							tryPath = std::filesystem::path(SplitString(fbxPath.filename().stem().string(), '-')[0]) / filename;
					}

					texData.handle = GetTextureManager().LoadTexture(tryPath.string());
					if (bgfx::isValid(texData.handle))
					{
						ui::Log("CreateMaterialFromFbx: loaded %s texture %s for %s: %s", Cstr(typeName), Cstr(texData.name), Cstr(materialName), PathStr(tryPath));
						texData.name = tryPath.string();
					}
					else
						ui::LogError("CreateMaterialFromFbx: failed %s file: %s for %s", Cstr(typeName), PathStr(tryPath), Cstr(materialName));
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
							ui::Log("CreateMaterialFromFbx: loaded embedded %s texture %s for %s", Cstr(typeName), Cstr(texData.name), Cstr(materialName));
						else
							ui::LogError("CreateMaterialFromFbx: failed embedded %s for %s", Cstr(typeName), Cstr(materialName));
						textures.push_back(texData);
					}
				}
			}
		}

		// load material with texture names
		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, {}, textures);
		ui::Log("material: %s %s %s %zu", Cstr(materialName), Cstr(vsName), Cstr(fsName), textures.size());

		// PBR-like properties
		ofbx::Color diffuse   = fbxMaterial->getDiffuseColor();
		ofbx::Color emissive  = fbxMaterial->getEmissiveColor();
		const float metallic  = fbxMaterial->getSpecularFactor();
		const float shininess = fbxMaterial->getShininess();
		const float roughness = (shininess > 0.0f) ? 1.0f - bx::sqrt(shininess / 100.0f) : 1.0f;
		material->SetPbrProperties(glm::vec4(diffuse.r, diffuse.g, diffuse.b, 1.0f), metallic, roughness);
		material->emissiveFactor = glm::vec3(emissive.r, emissive.g, emissive.b);

		// alpha and rendering state
		const int alphaMode   = (shininess < 1.0f) ? AlphaMode_Blend : AlphaMode_Opaque;
		material->alphaCutoff = 0.5f;
		material->alphaMode   = alphaMode;
		material->doubleSided = false;
		material->unlit       = !hasPbrTextures;

		// update render state based on alphaMode and doubleSided
		{
			uint64_t state = 0
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA
				| BGFX_STATE_WRITE_A
				| BGFX_STATE_WRITE_RGB
				| BGFX_STATE_WRITE_Z;
			if (alphaMode == AlphaMode_Blend || 1) state |= BGFX_STATE_BLEND_ALPHA; // COCKPIT HAS SOME ALPHA TEXTURE I GUESS => HOW TO DETECT THAT???
			if (!material->doubleSided) state |= BGFX_STATE_CULL_CW;
			material->state = state;
		}
	}
	// default material
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
	glm::mat4 glmMatrix = FbxToMatrix(fbxMatrix);

	mesh->matrix = glmMatrix;
	mesh->DecomposeMatrix(0.01f);
	mesh->UpdateLocalMatrix("CreateNodeMesh");

	// clang-format off
	mesh->aabb   = { { FLT_MAX, FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
	mesh->sphere = { { 0.0f, 0.0f, 0.0f }, 0.0f };
	// clang-format on

	ui::Log("CreateNodeMesh: %s", Cstr(nodeName));
	return mesh;
}

/// Process FBX node recursively
static sMesh ProcessMesh(const ofbx::IScene& scene, const ofbx::Mesh* fbxMesh, const std::filesystem::path& fbxPath, std::string_view texPath)
{
	// 1) create mesh
	sMesh      mesh     = CreateNodeMesh(fbxMesh);
	const auto nodeName = mesh->name;

	ui::Log("ProcessMesh: %s", Cstr(nodeName));
	if (nodeName.starts_with("off:")) return nullptr;
	if (nodeName.starts_with("shape:"))
	{
		ui::Log("SHAPE FOUND!");
		return nullptr;
	}

	// 2) process geometry
	const ofbx::GeometryData&  geom      = fbxMesh->getGeometryData();
	const ofbx::Vec3Attributes positions = geom.getPositions();
	const ofbx::Vec3Attributes normals   = geom.getNormals();
	const ofbx::Vec2Attributes uvs       = geom.getUVs();
	const ofbx::Vec4Attributes colors    = geom.getColors();
	const ofbx::Vec3Attributes tangents  = geom.getTangents();

	const bool hasTangents = (tangents.values && tangents.count == positions.count);

	// 3) vertex layout
	// clang-format off
	bgfx::VertexLayout layout;
	layout.begin()
		.add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float, true)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Tangent  , 4, bgfx::AttribType::Float)
		.end();
	// clang-format on
	mesh->layout = layout;

	// 4) process partitions (materials)
	const int numMaterial = fbxMesh->getMaterialCount();
	for (int partitionId = 0; partitionId < geom.getPartitionCount(); ++partitionId)
	{
		Group       group;
		const auto& partition = geom.getPartition(partitionId);

		// material
		if (partitionId < numMaterial)
		{
			const auto* material = fbxMesh->getMaterial(partitionId);
			group.material       = CreateMaterialFromFbx(scene, material, fbxPath, texPath);
			if (!group.material)
			{
				ui::LogError("ProcessMesh: Cannot create material for mesh %s partition %d", Cstr(nodeName), partitionId);
				continue;
			}
			else
			{
				ui::Log("ProcessMesh: partitionId=%d:", partitionId);
				for (int id = 0; id < TextureType_Count; ++id)
				{
					if (group.material->texNames[id].size())
						ui::Log("  %d: %s : %d", id, Cstr(group.material->texNames[id]), bgfx::isValid(group.material->textures[id]));
				}
			}
		}

		// vertices and indices + positions (for AABB)
		std::vector<uint32_t> indices;
		std::vector<Vertex>   vertices;
		std::vector<bx::Vec3> vpositions;

		// clang-format off
		bx::Aabb   groupAabb   = { { FLT_MAX, FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
		bx::Sphere groupSphere = { { 0.0f, 0.0f, 0.0f }, 0.0f };
		// clang-format on

		// process polygons
		for (int polygonId = 0; polygonId < partition.polygon_count; ++polygonId)
		{
			const auto& polygon = partition.polygons[polygonId];
			int         triIndices[128];
			const int   triCount = ofbx::triangulate(geom, polygon, triIndices);
			// ui::Log("Mesh %s partition %d polygon %d: %d indices", Cstr(nodeName), partitionId, polygonId, triCount);
			if (triCount % 3 != 0)
			{
				ui::LogError("ProcessMesh: Invalid triangulation for mesh %s partition %d polygon %d: %d indices", Cstr(nodeName), partitionId, polygonId, triCount);
				continue;
			}

			for (int i = 0; i < triCount; ++i)
			{
				const int vertexId = triIndices[i];
				if (vertexId < 0 || vertexId >= positions.count)
				{
					ui::LogError("ProcessMesh: Invalid vertex index %d for mesh %s partition %d polygon %d", vertexId, Cstr(nodeName), partitionId, polygonId);
					continue;
				}

				ofbx::Vec3 pos = positions.get(vertexId);

				Vertex vertex;
				vertex.position = FbxToPosition(pos);

				if (normals.values && vertexId < normals.count)
				{
					ofbx::Vec3 normal = normals.get(vertexId);
					vertex.normal     = FbxToNormal(normal);
				}
				if (uvs.values && vertexId < uvs.count)
				{
					ofbx::Vec2 uv = uvs.get(vertexId);
					vertex.uv     = glm::vec2(TO_FLOAT(uv.x), 1.0f - TO_FLOAT(uv.y));
				}
				if (colors.values && vertexId < colors.count)
				{
					ofbx::Vec4 color = colors.get(vertexId);
					vertex.color     = glm::vec4(TO_FLOAT(color.x), TO_FLOAT(color.y), TO_FLOAT(color.z), TO_FLOAT(color.w));
				}
				if (hasTangents)
				{
					ofbx::Vec3 tangent = tangents.get(vertexId);
					vertex.tangent     = glm::vec4(FbxToNormal(tangent), 1.0f);
				}

				vertices.push_back(vertex);
				indices.push_back(TO_UINT32(vertices.size() - 1));
				vpositions.push_back({ vertex.position.x, vertex.position.y, vertex.position.z });
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
			ui::LogError("ProcessMesh: No vertices for mesh %s partition %d", Cstr(nodeName), partitionId);
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
		if (vertices.size())
		{
			if (!hasTangents)
			{
				ui::LogInfo("ProcessMesh: Computing tangents for partition %d in mesh %s", partitionId, Cstr(nodeName));
				ComputeTangentsMikktspace(vertices, indices);
			}

			group.vertices = static_cast<uint8_t*>(bx::alloc(entry::getAllocator(), vertices.size() * sizeof(Vertex)));
			memcpy(group.vertices, vertices.data(), vertices.size() * sizeof(Vertex));
			group.numVertices = TO_UINT32(vertices.size());
			group.vbh         = bgfx::createVertexBuffer(bgfx::makeRef(group.vertices, vertices.size() * sizeof(Vertex)), layout, BGFX_BUFFER_NONE);
			ui::Log("ProcessMesh: mesh %s partition %d: %zu vertices", Cstr(nodeName), partitionId, vertices.size());
		}
		else
		{
			ui::LogError("ProcessMesh: No vertices for mesh %s partition %d", Cstr(nodeName), partitionId);
			continue;
		}

		if (indices.size())
		{
			group.indices = static_cast<uint32_t*>(bx::alloc(entry::getAllocator(), indices.size() * sizeof(uint32_t)));
			memcpy(group.indices, indices.data(), indices.size() * sizeof(uint32_t));
			group.numIndices = TO_UINT32(indices.size());
			group.ibh        = bgfx::createIndexBuffer(bgfx::makeRef(group.indices, indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
			ui::Log("ProcessMesh: mesh %s partition %d: %zu indices", Cstr(nodeName), partitionId, indices.size());
		}
		else
		{
			ui::LogError("ProcessMesh: No indices for mesh %s partition %d", Cstr(nodeName), partitionId);
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

	// 5) set default material for the mesh
	mesh->material  = CreateMaterialFromFbx(scene, nullptr, fbxPath, texPath);
	mesh->material0 = mesh->material;

	return mesh;
}

static bx::Aabb TransformAabb(const bx::Aabb& aabb, const glm::mat4& matrix)
{
	bx::Aabb transformedAabb = { { FLT_MAX, FLT_MAX, FLT_MAX }, { -FLT_MAX, -FLT_MAX, -FLT_MAX } };
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
// INTERFACE
////////////

sMesh LoadFbx(const std::filesystem::path& path, bool ramcopy, std::string_view texPath)
{
	if (!std::filesystem::exists(path))
	{
		ui::LogError("LoadFbx: File not found %s", PathStr(path));
		return nullptr;
	}

	ui::Log("LoadFbx: %s", PathStr(path));

	// load FBX file in binary
	std::string content = ReadData(path);
	if (content.empty())
	{
		ui::LogError("LoadFbx: Failed to read file %s", PathStr(path));
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
		ui::LogError("LoadFbx: Failed to load scene %s: %s", PathStr(path), ofbx::getError());
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

		sMesh mesh = ProcessMesh(*scene, fbxMesh, path, texPath);
		if (!mesh) continue;

		meshMap[fbxMesh->id] = mesh;
		ui::Log("LoadFbx: %d/%d name=%s", i, meshCount, Cstr(mesh->name));

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
				ui::Log("LoadFbx: parent node %s for mesh %s", Cstr(parentMesh->name), Cstr(mesh->name));
			}
		}

		// add mesh to parent
		parentMesh->AddChild(mesh);
		ui::Log("LoadFbx: add mesh %s to parent %s", Cstr(mesh->name), Cstr(parentMesh->name));

		// update parent AABBs up the hierarchy
		sMesh current = mesh;
		while (auto parent = current ? Mesh::SharedPtr(current->parent.lock()) : nullptr)
		{
			if (current->aabb.min.x >= current->aabb.max.x)
			{
				ui::LogError("LoadFbx: Invalid AABB for mesh %s", Cstr(current->name));
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

			ui::Log("LoadFbx: updated parent %s: AABB min=(%.2f %.2f %.2f), max=(%.2f %.2f %.2f)", Cstr(parent->name), parent->aabb.min.x, parent->aabb.min.y, parent->aabb.min.z, parent->aabb.max.x, parent->aabb.max.y, parent->aabb.max.z);

			current = parent;
		}
	}

	// log hierarchy
	for (const auto& child : rootMesh->children)
	{
		const auto sparent = child->parent.lock();
		ui::Log("LoadFbx: child %s: parent=%s groups=%zu children=%zu", Cstr(child->name), sparent ? Cstr(sparent->name) : "none", Mesh::SharedPtr(child)->groups.size(), child->children.size());
	}

	// clean up
	scene->destroy();

	// if it's a scene with one child => return that child
	return (rootMesh->children.size() == 1) ? Mesh::SharedPtr(rootMesh->children[0]) : rootMesh;
}
