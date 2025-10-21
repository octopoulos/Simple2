// GltfLoader.cpp
// @author octopoulos
// @version 2025-10-17

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

/// Save embedded image data to a temporary file and return its path
static std::string SaveEmbeddedImage(const fastgltf::sources::Vector& data, std::string_view name, const std::filesystem::path& gltfPath)
{
	if (!data.bytes.empty())
	{
		const auto    tempPath = std::filesystem::path("temp") / Format("%s.png", Cstr(name));
		std::ofstream out(tempPath, std::ios::binary);
		if (!out)
		{
			ui::LogError("Failed to create temporary file for embedded image: %s", PathStr(tempPath));
			return "";
		}
		out.write(reinterpret_cast<const char*>(data.bytes.data()), data.bytes.size());
		out.close();
		ui::Log("Saved embedded image to: %s", PathStr(tempPath));
		return tempPath.string();
	}
	return "";
}

/// Handle image data variant
static std::string ProcessImageData(const fastgltf::DataSource& data, std::string_view name, const std::filesystem::path& gltfPath)
{
	ui::Log("ProcessImageData: %s %s", Cstr(name), PathStr(gltfPath));
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

/// Create an sMaterial from a fastgltf::Material using MaterialManager and TextureManager
static sMaterial CreateMaterialFromGltf(const fastgltf::Asset& asset, std::optional<std::size_t> materialId, const std::filesystem::path& gltfPath, std::string_view texPath)
{
	// default shaders
	std::string      fsName       = "fs_pbr";//model_texture_normal";
	std::string      vsName       = "vs_pbr";//model_texture_normal";
	sMaterial        material     = nullptr;
	std::string      materialName = "DefaultMaterial";
	int              numTexture   = 0;
	VEC_STR          texNames     = {};
	VEC<TextureData> textures     = {};

	if (materialId.has_value() && materialId.value() < asset.materials.size())
	{
		const auto& mat = asset.materials[materialId.value()];
		materialName    = mat.name.empty() ? Format("Material_%d", materialId.value()) : mat.name;

		// select shaders based on material type
		if (mat.unlit)
		{
			// fsName = "fs_cube";
			// vsName = "vs_cube";
		}

		// PBR Base Color Texture
		if (mat.pbrData.baseColorTexture.has_value() && mat.pbrData.baseColorTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.pbrData.baseColorTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image   = asset.images[texture.imageIndex.value()];
				const auto  texName = ProcessImageData(image.data, Format("embedded_%d_baseColor", materialId.value()), gltfPath);
				if (texName.size()) texNames.push_back(texName);
			}
		}

		// normal Texture
		if (mat.normalTexture.has_value() && mat.normalTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.normalTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image   = asset.images[texture.imageIndex.value()];
				const auto  texName = ProcessImageData(image.data, Format("embedded_%d_normal", materialId.value()), gltfPath);
				if (texName.size()) texNames.push_back(texName);
			}
		}

		// emissive Texture
		if (mat.emissiveTexture.has_value() && mat.emissiveTexture->textureIndex < asset.textures.size())
		{
			const auto& texture = asset.textures[mat.emissiveTexture->textureIndex];
			if (texture.imageIndex.has_value() && texture.imageIndex.value() < asset.images.size())
			{
				const auto& image   = asset.images[texture.imageIndex.value()];
				const auto  texName = ProcessImageData(image.data, Format("embedded_%d_emissive", materialId.value()), gltfPath);
				if (texName.size()) texNames.push_back(texName);
			}
		}

		ui::Log("%d textures", numTexture);
		if (!numTexture)
		{
			fsName = "fs_cube";
			vsName = "vs_cube";
		}

		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, texNames);

		// PBR-like properties
		const auto& pbr = mat.pbrData;
		material->SetPbrProperties(
		    glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1], pbr.baseColorFactor[2], pbr.baseColorFactor[3]),
		    pbr.metallicFactor,
		    pbr.roughnessFactor
		);
		material->emissiveFactor = glm::vec3(mat.emissiveFactor[0], mat.emissiveFactor[1], mat.emissiveFactor[2]);
		material->occlusionStrength = mat.occlusionTexture.has_value() ? mat.occlusionTexture->strength : 1.0f;

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
		material = GetMaterialManager().LoadMaterial(materialName, "vs_model_color", "fs_model_color");
		material->SetPbrProperties(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.0f, 1.0f);

	}

	return material;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN
///////

sMesh LoadGltf(const std::filesystem::path& path, bool ramcopy, std::string_view texPath)
{
	fastgltf::Parser parser;

	// 1) load GLTF data buffer
	auto result = fastgltf::GltfDataBuffer::FromPath(path);
	if (result.error() != fastgltf::Error::None)
	{
		ui::LogError("LoadGltf: Buffer error %s", PathStr(path));
		return nullptr;
	}

	auto& data = result.get();
	ui::Log("data.totalSize=%d", data.totalSize());

	// 2) parse the GLTF asset
	auto gasset = parser.loadGltf(data, path.parent_path(), fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DontRequireValidAssetMember);
	if (const auto error = gasset.error(); error != fastgltf::Error::None)
	{
		ui::LogError("LoadGltf: Parse error %s %d", PathStr(path), TO_INT(error));
		return nullptr;
	}

	fastgltf::Asset& asset = gasset.get();
	ui::Log("LoadGltf: %s", PathStr(path));

	// 3) initialize sMesh
	sMesh mesh = std::make_shared<Mesh>(path.filename().string(), 0);

	// 4) vertex layout
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

	// 5) temporary vertex struct
	struct Vertex
	{
		glm::vec3 position = { 0.0f, 0.0f, 0.0f };
		glm::vec3 normal   = { 0.0f, 0.0f, 0.0f };
		glm::vec2 uv       = { 0.0f, 0.0f };
		glm::vec4 color    = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec4 tangent  = { 1.0f, 0.0f, 0.0f, 1.0f };
	};

	// log buffers
	for (const auto& buffer : asset.buffers)
		ui::Log("Buffer: %7d %s", buffer.byteLength, Cstr(buffer.name));

	// 6) iterate meshes
	for (const auto& gltfMesh : asset.meshes)
	{
		ui::Log("Mesh: %s", Cstr(gltfMesh.name));

		for (const auto& primitive : gltfMesh.primitives)
		{
			Group group;

			// assign material to group
			group.material = CreateMaterialFromGltf(asset, primitive.materialIndex, path, texPath);

			// === Indices ===
			std::vector<uint32_t> indices;
			if (primitive.indicesAccessor.has_value())
			{
				const auto& accessor = asset.accessors[primitive.indicesAccessor.value()];
				indices.reserve(accessor.count);
				fastgltf::iterateAccessor<uint32_t>(asset, accessor, [&](uint32_t idx) { indices.push_back(idx); });
				group.numIndices = static_cast<uint32_t>(indices.size());
				ui::Log("%zu indices", indices.size());
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
					ui::LogError("Accessor count mismatch for attribute %s", Cstr(attr.name));
					continue; // Skip invalid primitives
				}
			}

			// === vertices ===
			std::vector<Vertex> vertices(vertexCount);
			ui::Log("%zu vertices", vertices.size());

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

			// TANGENT
			if (auto it = primitive.findAttribute("TANGENT"); it != primitive.attributes.end())
			{
				const auto& accessor = asset.accessors[it->accessorIndex];
				fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, accessor, [&](fastgltf::math::fvec4 t, size_t i) {
					vertices[i].tangent = glm::vec4(t.x(), t.y(), t.z(), t.w());
				});
			}
			else
			{
				// Compute tangents for each triangle
				ui::Log("Computing tangents for primitive in mesh %s", Cstr(gltfMesh.name));
				for (size_t i = 0; i < indices.size(); i += 3)
				{
					if (i + 2 >= indices.size()) break;
					size_t v0 = indices[i];
					size_t v1 = indices[i + 1];
					size_t v2 = indices[i + 2];
					if (v0 >= vertices.size() || v1 >= vertices.size() || v2 >= vertices.size()) continue;

					glm::vec3 pos0 = vertices[v0].position;
					glm::vec3 pos1 = vertices[v1].position;
					glm::vec3 pos2 = vertices[v2].position;
					glm::vec2 uv0  = vertices[v0].uv;
					glm::vec2 uv1  = vertices[v1].uv;
					glm::vec2 uv2  = vertices[v2].uv;

					glm::vec3 edge1   = pos1 - pos0;
					glm::vec3 edge2   = pos2 - pos0;
					float     deltaU1 = uv1.x - uv0.x;
					float     deltaV1 = uv1.y - uv0.y;
					float     deltaU2 = uv2.x - uv0.x;
					float     deltaV2 = uv2.y - uv0.y;

					float     f       = deltaU1 * deltaV2 - deltaU2 * deltaV1;
					glm::vec4 tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
					if (fabs(f) > 1e-6) // Avoid division by zero
					{
						f                    = 1.0f / f;
						glm::vec3 tan        = f * (deltaV2 * edge1 - deltaV1 * edge2);
						glm::vec3 normal     = glm::normalize(glm::cross(edge1, edge2));
						glm::vec3 bitangent  = glm::cross(normal, tan);
						float     handedness = dot(bitangent, glm::cross(normal, tan)) > 0.0f ? 1.0f : -1.0f;
						tangent              = glm::vec4(glm::normalize(tan), handedness);
					}
					vertices[v0].tangent = tangent;
					vertices[v1].tangent = tangent;
					vertices[v2].tangent = tangent;
				}
			}

			// === Create BGFX buffers ===
			if (!vertices.empty())
			{
				group.vertices = reinterpret_cast<uint8_t*>(bx::alloc(entry::getAllocator(), vertices.size() * sizeof(Vertex)));
				memcpy(group.vertices, vertices.data(), vertices.size() * sizeof(Vertex));
				group.numVertices = static_cast<uint16_t>(vertices.size());
				group.vbh         = bgfx::createVertexBuffer(bgfx::makeRef(group.vertices, vertices.size() * sizeof(Vertex)), layout);
			}

			if (!indices.empty())
			{
				group.indices = reinterpret_cast<uint32_t*>(bx::alloc(entry::getAllocator(), indices.size() * sizeof(uint32_t)));
				for (size_t i = 0; i < indices.size(); ++i)
					group.indices[i] = static_cast<uint32_t>(indices[i]);
				group.numIndices = static_cast<uint32_t>(indices.size());
				group.ibh        = bgfx::createIndexBuffer(bgfx::makeRef(group.indices, indices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
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
	}

	// 7) set a default material for the mesh (optional, for fallback)
	mesh->material  = CreateMaterialFromGltf(asset, std::nullopt, path, texPath);
	mesh->material0 = mesh->material;

	return mesh;
}
