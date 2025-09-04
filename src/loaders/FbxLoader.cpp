// FbxLoader.cpp
// @author octopoulos
// @version 2025-08-30

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include <ofbx.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

static sMaterial CreateMaterialFromFbx(const ofbx::IScene& scene, const ofbx::Material* fbxMaterial, const std::filesystem::path& fbxPath)
{
	std::string materialName = "DefaultMaterial";
	std::string colorTextureName;
	std::string emissiveTextureName;
	std::string normalTextureName;

	std::string vsName     = "vs_model_color"; // Default to color-based lit shader
	std::string fsName     = "fs_model_color";
	sMaterial   material   = nullptr;
	int         numTexture = 0;

	if (fbxMaterial)
	{
		materialName = fbxMaterial->name[0] ? std::string(fbxMaterial->name) : fmt::format("Material_{}", fbxMaterial->id);

		// diffuse (base color) Texture
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
				colorTextureName               = std::filesystem::path(texturePath).filename().string();
				std::filesystem::path fullPath = fbxPath.parent_path() / colorTextureName;
				bgfx::TextureHandle   handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					colorTextureName.clear();
					ui::LogError("Failed to load diffuse texture: {}", fullPath.string());
				}
			}

			// check for embedded texture data
			if (colorTextureName.empty())
			{
				ofbx::DataView embeddedData = diffuseTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					colorTextureName           = fmt::format("embedded_{}_baseColor", fbxMaterial->id);
					bgfx::TextureHandle handle = GetTextureManager().AddTexture(colorTextureName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						colorTextureName.clear();
						ui::LogError("Failed to load embedded diffuse texture for material {}", materialName);
					}
				}
			}
		}

		// normal Texture
		if (const ofbx::Texture* normalTexture = fbxMaterial->getTexture(ofbx::Texture::NORMAL))
		{
			ofbx::DataView filename = normalTexture->getFileName();
			char           texturePath[256];
			filename.toString(texturePath);
			ui::Log("TEX: normal={}", texturePath);
			if (texturePath[0])
			{
				normalTextureName              = std::filesystem::path(texturePath).filename().string();
				std::filesystem::path fullPath = fbxPath.parent_path() / normalTextureName;
				bgfx::TextureHandle   handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					normalTextureName.clear();
					ui::LogError("Failed to load normal texture: {}", fullPath.string());
				}
			}

			// check for embedded normal texture
			if (normalTextureName.empty())
			{
				ofbx::DataView embeddedData = normalTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					normalTextureName          = fmt::format("embedded_{}_normal", fbxMaterial->id);
					bgfx::TextureHandle handle = GetTextureManager().AddTexture(normalTextureName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						normalTextureName.clear();
						ui::LogError("Failed to load embedded normal texture for material {}", materialName);
					}
				}
			}
		}

		// emissive Texture
		if (const ofbx::Texture* emissiveTexture = fbxMaterial->getTexture(ofbx::Texture::EMISSIVE))
		{
			ofbx::DataView filename = emissiveTexture->getFileName();
			char           texturePath[256];
			filename.toString(texturePath);
			ui::Log("TEX: emissive={}", texturePath);
			if (texturePath[0])
			{
				emissiveTextureName            = std::filesystem::path(texturePath).filename().string();
				std::filesystem::path fullPath = fbxPath.parent_path() / emissiveTextureName;
				bgfx::TextureHandle   handle   = GetTextureManager().LoadTexture(fullPath.string());
				if (bgfx::isValid(handle))
					++numTexture;
				else
				{
					emissiveTextureName.clear();
					ui::LogError("Failed to load emissive texture: {}", fullPath.string());
				}
			}

			// check for embedded emissive texture
			if (emissiveTextureName.empty())
			{
				ofbx::DataView embeddedData = emissiveTexture->getEmbeddedData();
				if (embeddedData.begin && embeddedData.end)
				{
					emissiveTextureName        = fmt::format("embedded_{}_emissive", fbxMaterial->id);
					bgfx::TextureHandle handle = GetTextureManager().AddTexture(emissiveTextureName, embeddedData.begin, TO_UINT32(embeddedData.end - embeddedData.begin));
					if (bgfx::isValid(handle))
						++numTexture;
					else
					{
						emissiveTextureName.clear();
						ui::LogError("Failed to load embedded emissive texture for material {}", materialName);
					}
				}
			}
		}

		// load material with texture names
		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, colorTextureName, normalTextureName);

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

	// Load FBX file using ReadData
	std::string content = ReadData(path, false); // Binary mode for FBX
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
		| ofbx::LoadFlags::IGNORE_VIDEOS
		;

	ofbx::IScene* scene = ofbx::load(reinterpret_cast<const ofbx::u8*>(content.data()), content.size(), static_cast<ofbx::u16>(flags));
	if (!scene)
	{
		ui::LogError("LoadFbx: Failed to load scene {}: {}", path.string(), ofbx::getError());
		return nullptr;
	}

	// initialize sMesh
	sMesh mesh = std::make_shared<Mesh>(path.filename().string(), 0);

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

	// process meshes
	for (int meshId = 0, numMesh = scene->getMeshCount(); meshId < numMesh; ++meshId)
	{
		const ofbx::Mesh*          fbxMesh   = scene->getMesh(meshId);
		const ofbx::GeometryData&  geom      = fbxMesh->getGeometryData();
		const ofbx::Vec3Attributes positions = geom.getPositions();
		const ofbx::Vec3Attributes normals   = geom.getNormals();
		const ofbx::Vec2Attributes uvs       = geom.getUVs();
		const ofbx::Vec4Attributes colors    = geom.getColors();

		// each mesh can have multiple partitions (materials)
		for (int partitionId = 0; partitionId < geom.getPartitionCount(); ++partitionId)
		{
			Group       group;
			const auto& partition = geom.getPartition(partitionId);

			// Material
			const auto* material = fbxMesh->getMaterial(partitionId);
			group.material       = CreateMaterialFromFbx(*scene, material, path);
			if (!group.material)
			{
				ui::LogError("Failed to create material for mesh {} partition {}", meshId, partitionId);
				continue;
			}

			// vertices and indices
			std::vector<Vertex>   vertices;
			std::vector<uint32_t> indices;

			// process polygons
			for (int polygonId = 0; polygonId < partition.polygon_count; ++polygonId)
			{
				const auto& polygon = partition.polygons[polygonId];
				int         triIndices[128]; // Max 42 triangles (128/3) per polygon
				const int   triCount = ofbx::triangulate(geom, polygon, triIndices);
				// ui::Log("Mesh {} partition {} polygon {}: {} indices", meshId, partitionId, polygonId, triCount);
				if (triCount % 3 != 0)
				{
					ui::LogError("Invalid triangulation for mesh {} partition {} polygon {}: {} indices", meshId, partitionId, polygonId, triCount);
					continue;
				}

				for (int i = 0; i < triCount; ++i)
				{
					int vertexId = triIndices[i];
					if (vertexId < 0 || vertexId >= positions.count)
					{
						ui::LogError("Invalid vertex index {} for mesh {} partition {} polygon {}", vertexId, meshId, partitionId, polygonId);
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
				ui::Log("Mesh {} partition {}: {} vertices", meshId, partitionId, vertices.size());
			}
			else
			{
				ui::LogError("No vertices for mesh {} partition {}", meshId, partitionId);
				continue;
			}

			if (!indices.empty())
			{
				group.indices = static_cast<uint32_t*>(bx::alloc(entry::getAllocator(), indices.size() * sizeof(uint32_t)));
				memcpy(group.indices, indices.data(), indices.size() * sizeof(uint32_t));
				group.numIndices = TO_UINT32(indices.size());
				group.ibh        = bgfx::createIndexBuffer(bgfx::makeRef(group.indices, indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
				ui::Log("Mesh {} partition {}: {} indices", meshId, partitionId, indices.size());
			}
			else
			{
				ui::LogError("No indices for mesh {} partition {}", meshId, partitionId);
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

		// apply node transform
		const auto fbxMatrix = fbxMesh->getGlobalTransform(); // DMatrix (double[16])
		glm::mat4  glmMatrix;
		for (int i = 0; i < 16; ++i)
			glmMatrix[i / 4][i % 4] = TO_FLOAT(fbxMatrix.m[i]);
		mesh->matrix = glmMatrix;
		mesh->UpdateWorldMatrix();
	}

	// default material
	mesh->material  = CreateMaterialFromFbx(*scene, nullptr, path);
	mesh->material0 = mesh->material;

	// clean up
	scene->destroy();
	return mesh;
}
