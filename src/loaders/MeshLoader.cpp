// MeshLoader.cpp
// @author octopoulos
// @version 2025-08-27

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

static const VEC_STR priorityExts = { "glb", "gltf", "bin" };

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GLTF
///////

// Save embedded image data to a temporary file and return its path
std::string SaveEmbeddedImage(const fastgltf::sources::Vector& data, const std::string& name, const std::filesystem::path& gltfPath)
{
	if (!data.bytes.empty())
	{
		const auto    tempPath = std::filesystem::path("temp") / fmt::format("{}.png", name);
		std::ofstream out(tempPath, std::ios::binary);
		if (!out)
		{
			ui::LogError("Failed to create temporary file for embedded image: {}", tempPath.string());
			return "";
		}
		out.write(reinterpret_cast<const char*>(data.bytes.data()), data.bytes.size());
		out.close();
		ui::Log("Saved embedded image to: {}", tempPath.string());
		return tempPath.string();
	}
	return "";
}

// Handle image data variant
std::string ProcessImageData(const fastgltf::DataSource& data, const std::string& name, const std::filesystem::path& gltfPath)
{
	ui::Log("ProcessImageData: {} {}", name, gltfPath);
	return std::visit(
	    [&](const auto& source) -> std::string {
		    using T = std::decay_t<decltype(source)>;
		    if constexpr (std::is_same_v<T, fastgltf::sources::URI>)
		    {
			    if (!source.uri.isDataUri() && source.uri.valid())
				    return source.uri.fspath().string();
		    }
		    else if constexpr (std::is_same_v<T, fastgltf::sources::Vector>)
		    {
			    return SaveEmbeddedImage(source, name, gltfPath);
		    }
		    return "";
	    },
	    data);
}

// Create an sMaterial from a fastgltf::Material using MaterialManager and TextureManager
sMaterial CreateMaterialFromGltf(const fastgltf::Asset& asset, std::optional<std::size_t> materialId, const std::filesystem::path& gltfPath)
{
	MaterialManager& materialManager = GetMaterialManager();
	TextureManager&  textureManager  = GetTextureManager();
	std::string      materialName    = "DefaultMaterial";
	std::string      colorTextureName;
	std::string      emissiveTextureName;
	std::string      normalTextureName;

	// default shaders
	std::string fsName     = "fs_model_texture_normal";
	std::string vsName     = "vs_model_texture_normal";
	sMaterial   material   = nullptr;
	int         numTexture = 0;

	if (materialId.has_value() && materialId.value() < asset.materials.size())
	{
		const auto& mat = asset.materials[materialId.value()];
		materialName    = mat.name.empty() ? fmt::format("Material_{}", materialId.value()) : std::string(mat.name);

		// select shaders based on material type
		if (mat.unlit)
		{
			fsName = "fs_cube";
			vsName = "vs_cube";
		}

		// PBR Base Color Texture
		if (mat.pbrData.baseColorTexture.has_value() && mat.pbrData.baseColorTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.pbrData.baseColorTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image = asset.images[texture.imageIndex.value()];
				colorTextureName  = ProcessImageData(image.data, fmt::format("embedded_{}_baseColor", materialId.value()), gltfPath);
				++numTexture;
			}
		}

		// normal Texture
		if (mat.normalTexture.has_value() && mat.normalTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.normalTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image = asset.images[texture.imageIndex.value()];
				normalTextureName = ProcessImageData(image.data, fmt::format("embedded_{}_normal", materialId.value()), gltfPath);
				++numTexture;
			}
		}

		// emissive Texture
		if (mat.emissiveTexture.has_value() && mat.emissiveTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.emissiveTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image   = asset.images[texture.imageIndex.value()];
				emissiveTextureName = ProcessImageData(image.data, fmt::format("embedded_{}_emissive", materialId.value()), gltfPath);
				++numTexture;
			}
		}

		ui::Log("{} textures", numTexture);
		if (!numTexture)
		{
			fsName = "fs_cube";
			vsName = "vs_cube";
		}

		material        = materialManager.LoadMaterial(materialName, vsName, fsName, colorTextureName, normalTextureName);
		const auto& pbr = mat.pbrData;

		// modes
		int alphaMode = AlphaMode_Opaque;
		if (mat.alphaMode == fastgltf::AlphaMode::Blend)
			alphaMode = AlphaMode_Blend;
		else if (mat.alphaMode == fastgltf::AlphaMode::Mask)
			alphaMode = AlphaMode_Mask;

		material->alphaCutoff = mat.alphaCutoff;
		material->alphaMode   = alphaMode;
		material->doubleSided = mat.doubleSided;
		material->unlit       = mat.unlit;

		// PBR
		material->SetPbrProperties(
		    glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]),
		    pbr.metallicFactor,
		    pbr.roughnessFactor
		);
		material->emissiveFactor = glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);

		// update render state based on alphaMode and doubleSided
		{
			uint64_t state = 0
			    | BGFX_STATE_DEPTH_TEST_LESS
			    | BGFX_STATE_MSAA
			    | BGFX_STATE_WRITE_A
			    | BGFX_STATE_WRITE_RGB
			    | BGFX_STATE_WRITE_Z;
			if (alphaMode == AlphaMode_Blend) state |= BGFX_STATE_BLEND_ALPHA;
			if (!material->doubleSided) state |= BGFX_STATE_CULL_CCW;
			material->state = state;
		}
	}
	// default material
	else
	{
		fsName   = "fs_cube";
		vsName   = "vs_cube";
		material = materialManager.LoadMaterial(materialName, vsName, fsName);
	}

	return material;
}

sMesh LoadGltf(const std::filesystem::path& path)
{
	fastgltf::Parser parser;

	// load GLTF data buffer
	auto result = fastgltf::GltfDataBuffer::FromPath(path);
	if (result.error() != fastgltf::Error::None)
	{
		ui::LogError("LoadGltf: Buffer error {}", path.string());
		return nullptr;
	}

	auto& data = result.get();
	ui::Log("data.totalSize={}", data.totalSize());

	// parse the GLTF asset
	auto gasset = parser.loadGltf(data, path.parent_path(), fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DontRequireValidAssetMember);
	if (gasset.error() != fastgltf::Error::None)
	{
		ui::LogError("LoadGltf: Parse error {}", path.string());
		return nullptr;
	}

	fastgltf::Asset& asset = gasset.get();
	ui::Log("LoadGltf: {}", path.string());

	// initialize sMesh
	sMesh mesh = std::make_shared<Mesh>(path.filename().string(), MeshLoad_Full);

	// build vertex layout (pos/norm/uv0 at least)
	bgfx::VertexLayout layout;
	// clang-format off
	layout.begin()
	    .add(bgfx::Attrib::Position , 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal   , 3, bgfx::AttribType::Float, true)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	// clang-format on

	mesh->layout = layout;

	// temporary vertex struct
	struct Vertex
	{
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 normal   = { 0.0f, 0.0f, 0.0f };
		glm::vec2 uv       = { 0.0f, 0.0f };
		glm::vec4 color    = { 1.0f, 1.0f, 1.0f, 1.0f };
	};

	// log buffers
	for (const auto& buffer : asset.buffers)
		ui::Log("Buffer: {:7} {}", buffer.byteLength, buffer.name);

	// iterate meshes
	for (const auto& gltfMesh : asset.meshes)
	{
		ui::Log("Mesh: {}", gltfMesh.name);

		for (const auto& primitive : gltfMesh.primitives)
		{
			Group group;

			// assign material to group
			group.material = CreateMaterialFromGltf(asset, primitive.materialIndex, path);

			// === Indices ===
			std::vector<uint32_t> indices;
			if (primitive.indicesAccessor.has_value())
			{
				const auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
				indices.reserve(accessor.count);
				fastgltf::iterateAccessor<uint32_t>(asset, accessor, [&](uint32_t idx) { indices.push_back(idx); });
				group.m_numIndices = static_cast<uint32_t>(indices.size());
				ui::Log("{} indices", indices.size());
			}

			// === vertex count from POSITION ===
			size_t vertexCount = 0;
			if (auto it = primitive.findAttribute("POSITION"); it != primitive.attributes.end())
				vertexCount = asset.accessors[it->accessorIndex].count;
			else
			{
				ui::LogError("Primitive missing POSITION attribute");
				continue; // Skip primitives without POSITION
			}

			// validate other accessors
			for (const auto& attr : primitive.attributes)
			{
				const auto& accessor = asset.accessors[attr.accessorIndex];
				if (accessor.count != vertexCount)
				{
					ui::LogError("Accessor count mismatch for attribute {}", attr.name);
					continue; // Skip invalid primitives
				}
			}

			// === vertices ===
			std::vector<Vertex> vertices(vertexCount);
			ui::Log("{} vertices", vertices.size());

			// POSITION
			if (auto it = primitive.findAttribute("POSITION"); it != primitive.attributes.end())
			{
				const auto& accessor = asset.accessors[it->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 pos, size_t i) {
					vertices[i].position = glm::vec3(pos.x(), pos.y(), pos.z());
				});
			}

			// NORMAL
			if (auto it = primitive.findAttribute("NORMAL"); it != primitive.attributes.end())
			{
				const auto& accessor = asset.accessors[it->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 n, size_t i) {
					vertices[i].normal = glm::vec3(n.x(), n.y(), n.z());
				});
			}

			// TEXCOORD_0
			if (auto it = primitive.findAttribute("TEXCOORD_0"); it != primitive.attributes.end())
			{
				const auto& accessor = asset.accessors[it->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, accessor, [&](fastgltf::math::fvec2 uv, size_t i) {
					vertices[i].uv = glm::vec2(uv.x(), uv.y());
				});
			}

			// COLOR_0
			if (auto it = primitive.findAttribute("COLOR_0"); it != primitive.attributes.end())
			{
				const auto& accessor = asset.accessors[it->accessorIndex];
				if (accessor.componentType == fastgltf::ComponentType::Float)
				{
					if (accessor.type == fastgltf::AccessorType::Vec3)
					{
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, accessor, [&](fastgltf::math::fvec3 c, size_t i) {
							vertices[i].color = glm::vec4(c.x(), c.y(), c.z(), 1.0f);
						});
					}
					else if (accessor.type == fastgltf::AccessorType::Vec4)
					{
						fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](fastgltf::math::fvec4 c, size_t i) {
							vertices[i].color = glm::vec4(c.x(), c.y(), c.z(), c.w());
						});
					}
				}
				else if (accessor.componentType == fastgltf::ComponentType::UnsignedByte)
				{
					fastgltf::iterateAccessorWithIndex<fastgltf::math::u8vec4>(asset, accessor, [&](fastgltf::math::u8vec4 c, size_t i) {
						vertices[i].color = glm::vec4(c.x(), c.y(), c.z(), c.w()) / 255.0f;
					});
				}
			}

			// === Create BGFX buffers ===
			if (!vertices.empty())
			{
				group.m_vertices = reinterpret_cast<uint8_t*>(bx::alloc(entry::getAllocator(), vertices.size() * sizeof(Vertex)));
				memcpy(group.m_vertices, vertices.data(), vertices.size() * sizeof(Vertex));
				group.m_numVertices = static_cast<uint16_t>(vertices.size());
				group.m_vbh         = bgfx::createVertexBuffer(bgfx::makeRef(group.m_vertices, vertices.size() * sizeof(Vertex)), layout);
			}

			if (!indices.empty())
			{
				group.m_indices = reinterpret_cast<uint16_t*>(bx::alloc(entry::getAllocator(), indices.size() * sizeof(uint16_t)));
				for (size_t i = 0; i < indices.size(); ++i)
					group.m_indices[i] = static_cast<uint16_t>(indices[i]);
				group.m_numIndices = static_cast<uint32_t>(indices.size());
				group.m_ibh        = bgfx::createIndexBuffer(bgfx::makeRef(group.m_indices, indices.size() * sizeof(uint16_t)));
			}

			// Set up primitive
			Primitive prim;
			prim.m_startIndex  = 0;
			prim.m_numIndices  = group.m_numIndices;
			prim.m_startVertex = 0;
			prim.m_numVertices = group.m_numVertices;
			group.m_prims.push_back(prim);

			mesh->groups.push_back(std::move(group));
		}
	}

	// Set a default material for the mesh (optional, for fallback)
	mesh->material  = CreateMaterialFromGltf(asset, std::nullopt, path);
	mesh->material0 = mesh->material;

	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MeshLoader
/////////////

sMesh MeshLoader::LoadModel(std::string_view name, std::string_view modelName, bool ramcopy)
{
	const std::filesystem::path modelDir = "runtime/models";

	sMesh mesh;

	for (const auto ext : priorityExts)
	{
		const auto path = modelDir / fmt::format("{}.{}", modelName, ext);
		if (IsFile(path))
		{
			// bgfx format
			if (ext == "bin")
			{
				bx::FilePath filePath(path.string().c_str());
				mesh = MeshLoad(name, filePath, ramcopy);
				if (mesh) break;
			}
			// gltf
			else
			{
				LoadGltf(path);
			}
		}
	}

	if (!mesh)
	{
		ui::LogError("LoadModel: Cannot load: {} @{}", name, modelName);
		return nullptr;
	}

	mesh->load      = MeshLoad_Basic;
	mesh->name      = name;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

sMesh MeshLoader::LoadModelFull(std::string_view name, std::string_view modelName, std::string_view textureName)
{
	// 1) load model
	auto mesh = LoadModel(name, modelName, true);
	if (!mesh) return nullptr;

	// 2) create material
	mesh->material = GetMaterialManager().LoadMaterial(modelName, "vs_model_texture", "fs_model_texture", textureName);
	mesh->material->FindModelTextures(modelName, textureName);

	// 3) done
	mesh->load      = MeshLoad_Full;
	mesh->modelName = NormalizeFilename(modelName);
	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

std::string NormalizeFilename(std::string_view filename)
{
	return ReplaceAll(filename, "\\", "/");
}

TEST_CASE("NormalizeFilename")
{
	// clang-format off
	const std::vector<std::tuple<std::string, std::string>> vectors = {
		{ ""               , ""               },
		{ "kenney/car.bin" , "kenney/car.bin" },
		{ "kenney\\car.bin", "kenney/car.bin" },
	};
	// clang-format on
	for (int i = -1; const auto& [filename, answer] : vectors)
	{
		SUBCASE_FMT("{}_{}", ++i, filename)
		CHECK(NormalizeFilename(filename) == answer);
	}
}
