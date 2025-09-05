// FbxLoader.cpp
// @author octopoulos
// @version 2025-09-01

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
	std::string colorName;
	std::string emissiveName;
	std::string normalName;

	std::string materialName = "DefaultMaterial";
	std::string vsName       = "vs_model_color"; // default to color-based lit shader
	std::string fsName       = "fs_model_color";
	sMaterial   material     = nullptr;
	int         numTexture   = 0;

	if (fbxMaterial)
	{
		materialName = fbxMaterial->name[0] ? std::string(fbxMaterial->name) : fmt::format("Material_{}", fbxMaterial->id);

		// diffuse (base color) texture
		if (const ofbx::Texture* diffuseTexture = fbxMaterial->getTexture(ofbx::Texture::DIFFUSE))
		{
			vsName                  = "vs_model_texture_normal";
			fsName                  = "fs_model_texture_normal";
			ofbx::DataView filename = diffuseTexture->getFileName();
			char           texturePath[256];
			filename.toString(texturePath);
			ui::Log("TEX: diffuse={}", texturePath);
			if (texturePath[0])
			{
				colorName           = std::filesystem::path(texturePath).filename().string();
				const auto fullPath = fbxPath.parent_path() / colorName;
				const auto handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					colorName.clear();
					ui::LogError("CreateMaterialFromFbx: Diffuse file: {}", fullPath.string());
				}
			}

			// check for embedded texture data
			if (colorName.empty())
			{
				ofbx::DataView embeddedData = diffuseTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					colorName         = fmt::format("embedded_{}_baseColor", fbxMaterial->id);
					const auto handle = GetTextureManager().AddTexture(colorName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						colorName.clear();
						ui::LogError("CreateMaterialFromFbx: Embedded diffuse for {}", materialName);
					}
				}
			}
		}

		// normal texture
		if (const ofbx::Texture* normalTexture = fbxMaterial->getTexture(ofbx::Texture::NORMAL))
		{
			ofbx::DataView filename = normalTexture->getFileName();
			char           texturePath[256];
			filename.toString(texturePath);
			ui::Log("TEX: normal={}", texturePath);
			if (texturePath[0])
			{
				normalName          = std::filesystem::path(texturePath).filename().string();
				const auto fullPath = fbxPath.parent_path() / normalName;
				const auto handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					normalName.clear();
					ui::LogError("CreateMaterialFromFbx: Normal file: {}", fullPath.string());
				}
			}

			// check for embedded normal texture
			if (normalName.empty())
			{
				ofbx::DataView embeddedData = normalTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					normalName        = fmt::format("embedded_{}_normal", fbxMaterial->id);
					const auto handle = GetTextureManager().AddTexture(normalName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						normalName.clear();
						ui::LogError("CreateMaterialFromFbx: Embedded normal for {}", materialName);
					}
				}
			}
		}

		// emissive texture
		if (const ofbx::Texture* emissiveTexture = fbxMaterial->getTexture(ofbx::Texture::EMISSIVE))
		{
			ofbx::DataView filename = emissiveTexture->getFileName();
			char           texturePath[256];
			filename.toString(texturePath);
			ui::Log("TEX: emissive={}", texturePath);
			if (texturePath[0])
			{
				emissiveName        = std::filesystem::path(texturePath).filename().string();
				const auto fullPath = fbxPath.parent_path() / emissiveName;
				const auto handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					emissiveName.clear();
					ui::LogError("CreateMaterialFromFbx: Emissive file: {}", fullPath.string());
				}
			}

			// check for embedded emissive texture
			if (emissiveName.empty())
			{
				ofbx::DataView embeddedData = emissiveTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					emissiveName      = fmt::format("embedded_{}_emissive", fbxMaterial->id);
					const auto handle = GetTextureManager().AddTexture(emissiveName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						emissiveName.clear();
						ui::LogError("CreateMaterialFromFbx: Embedded emissive for {}", materialName);
					}
				}
			}
		}

		// load material with texture names
		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, colorName, normalName);

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
	else material = GetMaterialManager().LoadMaterial(materialName, "vs_model_color", "fs_model_color");

	return material;
}

/// Create mesh for FBX node
static sMesh CreateNodeMesh(const ofbx::Object* node)
{
	std::string nodeName = node->name[0] ? std::string(node->name) : fmt::format("Node_{}", node->id);
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

	ui::Log("CreateNodeMesh: {}", nodeName);
	return mesh;
}

/// Process FBX node recursively
static sMesh ProcessMesh(const ofbx::IScene& scene, const ofbx::Mesh* fbxMesh, const std::filesystem::path& fbxPath)
{
	sMesh      mesh     = CreateNodeMesh(fbxMesh);
	const auto nodeName = mesh->name;

	ui::Log("ProcessMesh: {}", nodeName);

	// process geometry
	const ofbx::GeometryData&  geom      = fbxMesh->getGeometryData();
	const ofbx::Vec3Attributes positions = geom.getPositions();
	const ofbx::Vec3Attributes normals   = geom.getNormals();
	const ofbx::Vec2Attributes uvs       = geom.getUVs();
	const ofbx::Vec4Attributes colors    = geom.getColors();

	// build vertex layout
	bgfx::VertexLayout layout;
	// clang-format off
	layout.begin()
		.add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float, true)
		.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		.add(bgfx::Attrib::Color0   , 4, bgfx::AttribType::Float)
		.end();
	// clang-format on
	mesh->layout = layout;

	// vertex struct
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

	// process partitions (materials)
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

		// vertices and indices
		std::vector<Vertex>   vertices;
		std::vector<uint32_t> indices;

		// process polygons
		for (int polygonId = 0; polygonId < partition.polygon_count; ++polygonId)
		{
			const auto& polygon = partition.polygons[polygonId];
			int         triIndices[128]; // max 42 triangles (128/3) per polygon
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
		group.prims.push_back(prim);

		mesh->groups.push_back(std::move(group));
	}

	// set default material for the mesh
	mesh->material  = CreateMaterialFromFbx(scene, nullptr, fbxPath);
	mesh->material0 = mesh->material;

	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

sMesh LoadFbx(const std::filesystem::path& path)
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
	sMesh rootMesh = std::make_shared<Mesh>(path.filename().string(), 0);
	rootMesh->type |= ObjectType_Group;

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
				// create parent node
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
	}

	// log hierarchy
	for (const auto& child : rootMesh->children)
		ui::Log("LoadFbx: child {}: parent={} groups={} children={}", child->name, child->parent ? child->parent->name : "none", Mesh::SharedPtr(child)->groups.size(), child->children.size());

	// clean up
	scene->destroy();

	// if it's a scene with one child => return that child
	return (rootMesh->children.size() == 1) ? Mesh::SharedPtr(rootMesh->children[0]) : rootMesh;
}
