// FbxLoader2.cpp
// @author octopoulos
// @version 2025-10-31
//
// Full FBX loader following Godot's ufbx algorithm:
//  - triangulate faces with ufbx_triangulate_face()
//  - build per-corner attribute arrays
//  - patch tangent sign using vertex_bitangent (if present)
//  - SurfaceTool-style indexing/merging (quantized-hash)
//  - compute normals/tangents if missing
//
// Keeps your coding style and Vertex struct (uv + uv2), uses SDL3/bgfx/bullet conventions.

#include "stdafx.h"
#include "loaders/MeshLoader.h"
//
#include "core/common3d.h"             // ComputeTangentsMikktspace, Vertex
#include "materials/MaterialManager.h" // GetMaterialManager
#include "textures/TextureManager.h"   // GetTextureManager

#include "ufbx.h" // ufbx header

#include <vector>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <limits>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

static glm::vec3 ToVec3(const ufbx_vec3 &v) { return glm::vec3((float)v.x, (float)v.y, (float)v.z); }
static glm::vec2 ToVec2(const ufbx_vec2 &v) { return glm::vec2((float)v.x, (float)v.y); }
static glm::vec4 ToVec4(const ufbx_vec4 &v) { return glm::vec4((float)v.x, (float)v.y, (float)v.z, (float)v.w); }

static void ProcessFbxUvSet(std::vector<glm::vec2> &uv_array)
{
	// Godot-style: flip V
	for (size_t i = 0; i < uv_array.size(); ++i)
		uv_array[i].y = 1.0f - uv_array[i].y;
}

static inline uint64_t HashFloatQuant(float v)
{
	// Quantize float to reduce floating point equality issues when merging.
	// Quantization step chosen small enough for typical model precision.
	const int q = (int)std::floor(v * 100000.0f);
	return (uint64_t)(uint32_t)q;
}

static uint64_t HashVertexKey(const glm::vec3 &p, const glm::vec3 &n, const glm::vec2 &uv, const glm::vec2 &uv2, const glm::vec4 &col, float tanw)
{
	// Simple combined hash of quantized attributes (FNV-like mixing)
	uint64_t h = 1469598103934665603ULL; // FNV offset
	auto fnv1 = [&](uint64_t v) {
		h ^= v;
		h *= 1099511628211ULL;
	};
	// position
	fnv1(HashFloatQuant(p.x)); fnv1(HashFloatQuant(p.y)); fnv1(HashFloatQuant(p.z));
	// normal
	fnv1(HashFloatQuant(n.x)); fnv1(HashFloatQuant(n.y)); fnv1(HashFloatQuant(n.z));
	// uv
	fnv1(HashFloatQuant(uv.x)); fnv1(HashFloatQuant(uv.y));
	// uv2
	fnv1(HashFloatQuant(uv2.x)); fnv1(HashFloatQuant(uv2.y));
	// color
	fnv1(HashFloatQuant(col.x)); fnv1(HashFloatQuant(col.y)); fnv1(HashFloatQuant(col.z)); fnv1(HashFloatQuant(col.w));
	// tangent sign
	fnv1(HashFloatQuant(tanw));
	return h;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MATERIAL (unchanged)
///////////

static sMaterial CreateMaterialFromFbx(const ufbx_scene* scene, const ufbx_material* fbxMaterial, const std::filesystem::path& fbxPath, std::string_view texPath)
{
	const auto       baseName     = fbxPath.stem().string();
	std::string      fsName       = "fs_pbr";
	std::string      vsName       = "vs_pbr";
	sMaterial        material     = nullptr;
	std::string      materialName = "DefaultMaterial";
	VEC<TextureData> textures     = {};

	if (fbxMaterial)
	{
		materialName = fbxMaterial->name.length
		    ? FormatStr("%s:%s", Cstr(baseName), fbxMaterial->name.data)
		    : FormatStr("%s:#%zu", Cstr(baseName), fbxMaterial->element_id);

		bool hasPbrTextures = false;

		struct TexMap
		{
			int           type;
			ufbx_texture* tex;
		};

		TexMap texMaps[] = {
			{ TextureType_Diffuse,   fbxMaterial->pbr.base_color.texture        },
			{ TextureType_Normal,    fbxMaterial->pbr.normal_map.texture        },
			{ TextureType_Specular,  fbxMaterial->pbr.metalness.texture         },
			{ TextureType_Emissive,  fbxMaterial->pbr.emission_color.texture    },
			{ TextureType_Occlusion, fbxMaterial->pbr.ambient_occlusion.texture },
		};

		for (const auto& tm : texMaps)
		{
			ufbx_texture* tex = tm.tex;
			if (!tex) continue;

			const auto typeName = TextureName(tm.type);
			if (tm.type == TextureType_Diffuse || tm.type == TextureType_Normal || tm.type == TextureType_Specular || tm.type == TextureType_Occlusion)
			{
				vsName         = "vs_pbr";
				fsName         = "fs_pbr";
				hasPbrTextures = true;
			}

			std::string texFile;
			if (tex->filename.length) texFile.assign(tex->filename.data, tex->filename.length);
			else if (tex->relative_filename.length) texFile.assign(tex->relative_filename.data, tex->relative_filename.length);

			ui::Log("TEX: %s=%s for %s", Cstr(typeName), Cstr(texFile), Cstr(materialName));

			TextureData texData = { tm.type, "" };
			if (!texFile.empty())
			{
				auto       tryPath  = std::filesystem::path(NormalizeFilename(texFile));
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

			if (texData.name.empty() && tex->content.size)
			{
				texData.name   = Format("embedded_%zu_%s", tex->element_id, Cstr(typeName));
				texData.handle = GetTextureManager().AddRawTexture(
				    texData.name,
				    tex->content.data,
				    (uint32_t)tex->content.size);
				if (bgfx::isValid(texData.handle))
					ui::Log("CreateMaterialFromFbx: loaded embedded %s texture %s for %s", Cstr(typeName), Cstr(texData.name), Cstr(materialName));
				else
					ui::LogError("CreateMaterialFromFbx: failed embedded %s for %s", Cstr(typeName), Cstr(materialName));
				textures.push_back(texData);
			}
		}

		material = GetMaterialManager().LoadMaterial(materialName, vsName, fsName, {}, textures);
		ui::Log("material: %s %s %s %zu", Cstr(materialName), Cstr(vsName), Cstr(fsName), textures.size());

		// PBR properties
		const ufbx_vec3 diffCol   = fbxMaterial->pbr.base_color.value_vec3;
		const float     diffFact  = fbxMaterial->pbr.base_factor.value_real;
		const ufbx_vec3 emiCol    = fbxMaterial->pbr.emission_color.value_vec3;
		const float     emiFact   = fbxMaterial->pbr.emission_factor.value_real;
		const float     metal     = fbxMaterial->pbr.metalness.value_real;
		const float     rough     = fbxMaterial->pbr.roughness.value_real;
		const float     shininess = fbxMaterial->fbx.specular_factor.value_real;

		float finalRough = rough > 0.0f ? rough
		                                : (shininess > 0.0f ? 1.0f - bx::sqrt(shininess / 100.0f) : 1.0f);

		material->SetPbrProperties(
		    glm::vec4(diffCol.x * diffFact, diffCol.y * diffFact, diffCol.z * diffFact, 1.0f),
		    metal,
		    finalRough);

		material->emissiveFactor = glm::vec3(emiCol.x * emiFact, emiCol.y * emiFact, emiCol.z * emiFact);

		const int alphaMode         = (shininess < 1.0f) ? AlphaMode_Blend : AlphaMode_Opaque;
		material->alphaCutoff       = 0.5f;
		material->alphaMode         = alphaMode;
		material->doubleSided       = false;
		material->occlusionStrength = 1.0f;
		material->unlit             = !hasPbrTextures;

		uint64_t state = 0
		    | BGFX_STATE_DEPTH_TEST_LESS
		    | BGFX_STATE_MSAA
		    | BGFX_STATE_WRITE_A
		    | BGFX_STATE_WRITE_RGB
		    | BGFX_STATE_WRITE_Z;
		if (alphaMode == AlphaMode_Blend) state |= BGFX_STATE_BLEND_ALPHA;
		// if (!material->doubleSided) state |= BGFX_STATE_CULL_CW;
		material->state = state;
	}
	else
	{
		material = GetMaterialManager().LoadMaterial(materialName, "vs_model_color", "fs_model_color");
		material->SetPbrProperties(glm::vec4(0.8f, 0.8f, 0.8f, 1.0f), 0.0f, 1.0f);
	}
	return material;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NODE / MESH
//////////////

static sMesh CreateNodeMesh(const ufbx_node* node)
{
	const char* nodeName = node->name.length ? node->name.data : Format("Node_%zu", node->element_id);
	sMesh       mesh     = std::make_shared<Mesh>(nodeName, 0);
	mesh->type |= ObjectType_Mesh;

	// Use node_to_world matrix (already converted by ufbx when using MODIFY_GEOMETRY)
	const ufbx_matrix wm = node->node_to_world;
	mesh->matrix = glm::mat4(
		float(wm.m00), float(wm.m01), float(wm.m02), float(wm.m03),
		float(wm.m10), float(wm.m11), float(wm.m12), float(wm.m13),
		float(wm.m20), float(wm.m21), float(wm.m22), float(wm.m23),
		0.0f,          0.0f,          0.0f,          1.0f
	);

	mesh->DecomposeMatrix(0.01f);
	mesh->UpdateLocalMatrix("CreateNodeMesh");

	mesh->aabb = {
		{ FLT_MAX,  FLT_MAX,  FLT_MAX  },
		{ -FLT_MAX, -FLT_MAX, -FLT_MAX }
	};
	mesh->sphere = {
		{ 0.0f, 0.0f, 0.0f },
		0.0f
	};

	ui::Log("CreateNodeMesh: %s", Cstr(nodeName));
	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Indexing / SurfaceTool-like merging
//
// Build unique vertices from per-corner attribute arrays.
// Uses simple quantization hashing to merge nearly-identical floats.
///////

static void BuildIndexedMesh(
    const std::vector<glm::vec3>& posArr,
    const std::vector<glm::vec3>& normArr,
    const std::vector<glm::vec2>& uvArr,
    const std::vector<glm::vec2>& uv2Arr,
    const std::vector<glm::vec4>& colorArr,
    const std::vector<glm::vec4>& tangentArr, // w included
    std::vector<Vertex>& outVertices,
    std::vector<uint32_t>& outIndices)
{
	outVertices.clear();
	outIndices.clear();
	outVertices.reserve(posArr.size());
	outIndices.reserve(posArr.size());

	std::unordered_map<uint64_t, uint32_t> map;
	map.reserve(posArr.size() * 2 + 1);

	for (size_t i = 0; i < posArr.size(); ++i)
	{
		const glm::vec3 &p = posArr[i];
		const glm::vec3 &n = (i < normArr.size()) ? normArr[i] : glm::vec3(0.0f, 0.0f, 1.0f);
		const glm::vec2 &uv = (i < uvArr.size()) ? uvArr[i] : glm::vec2(0.0f, 0.0f);
		const glm::vec2 &uv2 = (i < uv2Arr.size()) ? uv2Arr[i] : glm::vec2(0.0f, 0.0f);
		const glm::vec4 &col = (i < colorArr.size()) ? colorArr[i] : glm::vec4(1.0f);
		const glm::vec4 &tan = (i < tangentArr.size()) ? tangentArr[i] : glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		float tanw = tan.w;

		uint64_t key = HashVertexKey(p, n, uv, uv2, col, tanw);
		auto it = map.find(key);
		if (it != map.end())
		{
			outIndices.push_back(it->second);
			continue;
		}

		// create new vertex
		Vertex v{};
		v.position = p;
		v.normal   = n;
		v.uv       = uv;
		v.uv2      = uv2;
		v.color    = col;
		v.tangent  = tan;

		uint32_t newIndex = (uint32_t)outVertices.size();
		outVertices.push_back(v);
		map.emplace(key, newIndex);
		outIndices.push_back(newIndex);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ProcessMesh — Godot algorithm: triangulate into corner-indices array,
// decode per-corner attribute arrays, patch tangent sign if bitangent present,
// then index/merge (SurfaceTool-like), compute tangents if needed.
///////

static sMesh ProcessMesh(const ufbx_scene* scene, const ufbx_mesh* fbxMesh, const std::filesystem::path& fbxPath, std::string_view texPath)
{
	if (fbxMesh->instances.count == 0) return nullptr;
	const ufbx_node* node = fbxMesh->instances.data[0];

	sMesh mesh = CreateNodeMesh(node);
	const auto nodeName = mesh->name;

	ui::Log("ProcessMesh: %s", Cstr(nodeName));
	if (nodeName.starts_with("off:")) return nullptr;
	if (nodeName.starts_with("shape:"))
	{
		ui::Log("SHAPE FOUND!");
		return nullptr;
	}

	// Vertex layout for bgfx
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float, true)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord1, 2, bgfx::AttribType::Float) // uv2 slot usage in your pipeline
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Tangent, 4, bgfx::AttribType::Float)
	    .end();
	mesh->layout = layout;

	// iterate material parts (submeshes)
	for (size_t partIdx = 0; partIdx < fbxMesh->material_parts.count; ++partIdx)
	{
		const ufbx_mesh_part& part = fbxMesh->material_parts.data[partIdx];
		Group group;

		// assign material (fallback to default)
		if (partIdx < fbxMesh->materials.count)
		{
			const ufbx_material* mat = fbxMesh->materials.data[partIdx];
			group.material           = CreateMaterialFromFbx(scene, mat, fbxPath, texPath);
		}
		else
			group.material = CreateMaterialFromFbx(scene, nullptr, fbxPath, texPath);

		// --- Build corner-index list the same way Godot does ---
		uint32_t num_triangles = 0;
		for (uint32_t face_index : part.face_indices)
		{
			const ufbx_face &face = fbxMesh->faces.data[face_index];
			if (face.num_indices >= 3) {
				num_triangles += (uint32_t)(face.num_indices - 2);
			}
		}
		if (num_triangles == 0)
		{
			ui::LogInfo("ProcessMesh: part %zu has no triangles, skipping", partIdx);
			continue;
		}

		std::vector<uint32_t> cornerIndices;
		cornerIndices.resize(num_triangles * 3);
		uint32_t offset = 0;

		for (uint32_t face_index : part.face_indices)
		{
			const ufbx_face face = fbxMesh->faces.data[face_index];
			if (face.num_indices < 3) continue;

			uint32_t* dst = cornerIndices.data() + offset;
			size_t space = cornerIndices.size() - offset;
			uint32_t found = ufbx_triangulate_face(dst, space, fbxMesh, face);
			// Godot uses clockwise winding: swap 0/2 for each triangle
			for (uint32_t t = 0; t < found; ++t)
			{
				std::swap(dst[t * 3 + 0], dst[t * 3 + 2]);
			}
			offset += found * 3;
		}

		// shrink if needed
		if (offset != (uint32_t)cornerIndices.size())
			cornerIndices.resize(offset);

		const uint32_t vertex_num = (uint32_t)cornerIndices.size();
		if (vertex_num == 0)
		{
			ui::LogError("ProcessMesh: No triangles after triangulation for part %zu of mesh %s", partIdx, Cstr(nodeName));
			continue;
		}

		// --- Decode per-corner attribute arrays ---
		std::vector<glm::vec3> posArr(vertex_num);
		std::vector<glm::vec3> normArr(vertex_num);
		std::vector<glm::vec2> uvArr(vertex_num);
		std::vector<glm::vec2> uv2Arr; uv2Arr.reserve(vertex_num); // maybe empty
		std::vector<glm::vec4> colorArr(vertex_num);
		std::vector<glm::vec4> tangentArr(vertex_num);

		// Flags for attributes existence
		const bool hasNormals     = fbxMesh->vertex_normal.exists;
		const bool hasUVs         = fbxMesh->vertex_uv.exists;
		const bool hasColors      = fbxMesh->vertex_color.exists;
		const bool hasTangents    = fbxMesh->vertex_tangent.exists;
		const bool hasBitangents  = fbxMesh->vertex_bitangent.exists;

		// If second UV set exists prepare uv2Arr
		bool hasUvSet1 = (fbxMesh->uv_sets.count >= 2) && fbxMesh->uv_sets[1].vertex_uv.exists;
		if (hasUvSet1) uv2Arr.resize(vertex_num);

		for (uint32_t i = 0; i < vertex_num; ++i)
		{
			uint32_t corner = cornerIndices[i];                        // corner index into vertex_indices
			uint32_t pos_idx = fbxMesh->vertex_indices.data[corner];  // vertex index for positions

			// Positions: from vertices[pos_idx]
			ufbx_vec3 posv = ufbx_get_vertex_vec3(&fbxMesh->vertex_position, pos_idx);
			posArr[i] = ToVec3(posv);

			// Normals - decode using corner index (handles per-corner or per-vertex)
			if (hasNormals)
			{
				ufbx_vec3 nv = ufbx_get_vertex_vec3(&fbxMesh->vertex_normal, corner);
				normArr[i] = ToVec3(nv);
				if (glm::dot(normArr[i], normArr[i]) > 1e-6f) normArr[i] = glm::normalize(normArr[i]);
			}
			else
			{
				normArr[i] = glm::vec3(0.0f);
			}

			// UVs - primary set (per-corner)
			if (hasUVs)
			{
				ufbx_vec2 uv = ufbx_get_vertex_vec2(&fbxMesh->vertex_uv, corner);
				uvArr[i] = ToVec2(uv);
			}
			else
			{
				uvArr[i] = glm::vec2(0.0f);
			}

			// UV set 1 (second uv) if present
			if (hasUvSet1)
			{
				// the second uv set uses a different accessor in ufbx: uv_sets[1].vertex_uv
				// use ufbx_get_vertex_vec2 on that accessor using corner index
				ufbx_vec2 uv2 = ufbx_get_vertex_vec2(&fbxMesh->uv_sets.data[1].vertex_uv, corner);
				uv2Arr[i] = ToVec2(uv2);
			}

			// Colors - per-corner
			if (hasColors)
			{
				ufbx_vec4 cv = ufbx_get_vertex_vec4(&fbxMesh->vertex_color, corner);
				colorArr[i] = ToVec4(cv);
			}
			else
			{
				colorArr[i] = glm::vec4(1.0f);
			}

			// Tangents - per-corner if exist. Keep w=1 for bitangent sign; patched later.
			if (hasTangents)
			{
				ufbx_vec3 tv = ufbx_get_vertex_vec3(&fbxMesh->vertex_tangent, corner);
				tangentArr[i] = glm::vec4((float)tv.x, (float)tv.y, (float)tv.z, 1.0f);
			}
			else
			{
				tangentArr[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			}
		}

		// Process additional uv_sets beyond 1 as Godot does: pack into custom arrays.
		// We only support uv2 -> Vertex::uv2 here; other sets are ignored (could be extended).
		if (hasUVs)
		{
			// Flip V on decoded UV arrays (Godot _process_uv_set)
			ProcessFbxUvSet(uvArr);
		}
		if (hasUvSet1)
		{
			ProcessFbxUvSet(uv2Arr);
		}

		// --- Patch tangent sign using vertex_bitangent like Godot does (before indexing) ---
		if (hasTangents && hasBitangents)
		{
			for (uint32_t i = 0; i < vertex_num; ++i)
			{
				glm::vec3 tangent = glm::vec3(tangentArr[i].x, tangentArr[i].y, tangentArr[i].z);
				glm::vec3 normal  = normArr[i];
				// generated bitangent
				glm::vec3 gen_bitangent = glm::cross(normal, tangent);
				// actual bitangent from FBX (per-corner)
				ufbx_vec3 bv = ufbx_get_vertex_vec3(&fbxMesh->vertex_bitangent, cornerIndices[i]);
				glm::vec3 bitangent = ToVec3(bv);
				// if dot < 0 -> flip sign
				if (glm::dot(gen_bitangent, bitangent) < 0.0f)
				{
					tangentArr[i].w = -1.0f;
				}
			}
		}

		// --- Index / merge vertices (SurfaceTool->index equivalent) ---
		std::vector<Vertex> finalVertices;
		std::vector<uint32_t> finalIndices;
		BuildIndexedMesh(posArr, normArr, uvArr, uv2Arr, colorArr, tangentArr, finalVertices, finalIndices);

		// If normals are missing, generate them (accumulation)
		bool needNormals = false;
		for (auto &vv : finalVertices) { if (glm::dot(vv.normal, vv.normal) < 1e-8f) { needNormals = true; break; } }
		if (needNormals)
		{
			ui::LogInfo("ProcessMesh: Generating normals for part %zu in mesh %s", partIdx, Cstr(nodeName));
			for (size_t i = 0; i + 2 < finalIndices.size(); i += 3)
			{
				Vertex &a = finalVertices[finalIndices[i + 0]];
				Vertex &b = finalVertices[finalIndices[i + 1]];
				Vertex &c = finalVertices[finalIndices[i + 2]];
				glm::vec3 faceN = glm::normalize(glm::cross(b.position - a.position, c.position - a.position));
				if (glm::dot(faceN, faceN) > 1e-8f)
				{
					a.normal += faceN;
					b.normal += faceN;
					c.normal += faceN;
				}
			}
			for (auto &vv : finalVertices) {
				if (glm::dot(vv.normal, vv.normal) > 1e-8f) vv.normal = glm::normalize(vv.normal);
				else vv.normal = glm::vec3(0.0f, 0.0f, 1.0f);
			}
		}

		// Compute tangents if missing or invalid
		bool hasValidTangents = false;
		for (auto &vv : finalVertices) {
			const glm::vec3 t = glm::vec3(vv.tangent.x, vv.tangent.y, vv.tangent.z);
			if (glm::dot(t, t) > 1e-8f) { hasValidTangents = true; break; }
		}
		if (!hasValidTangents && !finalVertices.empty())
		{
			ui::LogInfo("ProcessMesh: Computing tangents (MikkTSpace) for part %zu in mesh %s", partIdx, Cstr(nodeName));
			ComputeTangentsMikktspace(finalVertices, finalIndices);
		}

		// Build vpositions for bounds from finalVertices
		std::vector<bx::Vec3> vpositions; vpositions.reserve(finalVertices.size());
		for (const auto &fv : finalVertices) vpositions.push_back({ fv.position.x, fv.position.y, fv.position.z });

		// Bounds
		bx::Aabb groupAabb;
		bx::toAabb(groupAabb, vpositions.data(), (uint32_t)vpositions.size(), sizeof(bx::Vec3));
		bx::Sphere groupSphere;
		bx::calcMaxBoundingSphere(groupSphere, vpositions.data(), (uint32_t)vpositions.size(), sizeof(bx::Vec3));
		group.aabb   = groupAabb;
		group.sphere = groupSphere;

		mesh->aabb.min = bx::min(mesh->aabb.min, groupAabb.min);
		mesh->aabb.max = bx::max(mesh->aabb.max, groupAabb.max);
		{
			bx::Sphere tmp;
			bx::calcMaxBoundingSphere(tmp, vpositions.data(), (uint32_t)vpositions.size(), sizeof(bx::Vec3));
			if (tmp.radius > mesh->sphere.radius) mesh->sphere = tmp;
		}

		// Create bgfx buffers (copy into allocator memory)
		if (!finalVertices.empty())
		{
			group.vertices = (uint8_t*)bx::alloc(entry::getAllocator(), finalVertices.size() * sizeof(Vertex));
			memcpy(group.vertices, finalVertices.data(), finalVertices.size() * sizeof(Vertex));
			group.numVertices = (uint32_t)finalVertices.size();
			group.vbh = bgfx::createVertexBuffer(bgfx::makeRef(group.vertices, finalVertices.size() * sizeof(Vertex)), layout);
		}
		if (!finalIndices.empty())
		{
			group.indices = (uint32_t*)bx::alloc(entry::getAllocator(), finalIndices.size() * sizeof(uint32_t));
			memcpy(group.indices, finalIndices.data(), finalIndices.size() * sizeof(uint32_t));
			group.numIndices = (uint32_t)finalIndices.size();
			group.ibh = bgfx::createIndexBuffer(bgfx::makeRef(group.indices, finalIndices.size() * sizeof(uint32_t)), BGFX_BUFFER_INDEX32);
		}

		// Single primitive covering this group
		Primitive prim {};
		prim.startIndex  = 0;
		prim.numIndices  = group.numIndices;
		prim.startVertex = 0;
		prim.numVertices = group.numVertices;
		prim.aabb        = groupAabb;
		prim.sphere      = groupSphere;
		group.prims.push_back(prim);

		mesh->groups.push_back(std::move(group));
	} // parts

	// fallback material on mesh
	mesh->material  = CreateMaterialFromFbx(scene, nullptr, fbxPath, texPath);
	mesh->material0 = mesh->material;

	return mesh;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Transform AABB helper (kept from your code)
///////

static bx::Aabb TransformAabb(const bx::Aabb& aabb, const glm::mat4& matrix)
{
	bx::Aabb out = {
		{ FLT_MAX,  FLT_MAX,  FLT_MAX  },
		{ -FLT_MAX, -FLT_MAX, -FLT_MAX }
	};
	if (aabb.min.x >= aabb.max.x) return out;

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

	for (int i = 0; i < 8; ++i)
	{
		glm::vec4 c = matrix * glm::vec4(corners[i].x, corners[i].y, corners[i].z, 1.0f);
		corners[i]  = { c.x, c.y, c.z };
	}
	bx::toAabb(out, corners, 8, sizeof(bx::Vec3));
	return out;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry: LoadFbx (full loader with ufbx options tuned to bake geometry transforms)
///////

sMesh LoadFbx(const std::filesystem::path& path, bool ramcopy, std::string_view texPath)
{
	const auto pathStr = path.string();
	if (!std::filesystem::exists(path))
	{
		ui::LogError("LoadFbx: File not found %s", PathStr(path));
		return nullptr;
	}
	ui::Log("LoadFbx: %s", PathStr(path));

	std::string content = ReadData(path);
	if (content.empty())
	{
		ui::LogError("LoadFbx: Failed to read file %s", PathStr(path));
		return nullptr;
	}

	ufbx_load_opts opts = {};
	// Match Godot's robust defaults: bake geometry transforms/pivots into geometry
	opts.target_axes = ufbx_axes_right_handed_y_up;
	// opts.target_unit_meters = 1.0;
	opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	opts.geometry_transform_handling = UFBX_GEOMETRY_TRANSFORM_HANDLING_MODIFY_GEOMETRY_NO_FALLBACK;
	opts.inherit_mode_handling = UFBX_INHERIT_MODE_HANDLING_COMPENSATE_NO_FALLBACK;
	opts.pivot_handling = UFBX_PIVOT_HANDLING_ADJUST_TO_PIVOT;
	opts.generate_missing_normals = true;
	opts.clean_skin_weights = true;
	opts.filename = { Cstr(pathStr), pathStr.size() };
	opts.use_blender_pbr_material = true;
	opts.normalize_normals = true;
	opts.normalize_tangents = true;
	opts.retain_vertex_attrib_w = true;
	opts.skip_mesh_parts = false;
	opts.skip_skin_vertices = true;
	opts.ignore_animation = true;
	opts.ignore_embedded = false;
	opts.ignore_missing_external_files = true;
	opts.load_external_files = false;

	ufbx_error error;
	ufbx_scene* scene = ufbx_load_memory(content.data(), content.size(), &opts, &error);
	if (!scene)
	{
		ui::LogError("LoadFbx: Failed to load scene %s: %s", PathStr(path), error.description.data);
		return nullptr;
	}

	sMesh rootMesh = std::make_shared<Mesh>(path.filename().string(), ObjectType_Group);
	std::unordered_map<uint64_t, sMesh> meshMap;
	meshMap[rootMesh->id] = rootMesh;

	for (size_t i = 0; i < scene->meshes.count; ++i)
	{
		const ufbx_mesh* fbxMesh = scene->meshes.data[i];
		if (fbxMesh->instances.count == 0) continue;

		sMesh mesh = ProcessMesh(scene, fbxMesh, path, texPath);
		if (!mesh) continue;

		const ufbx_node* node = fbxMesh->instances.data[0];
		meshMap[node->element_id] = mesh;
		ui::Log("LoadFbx: %zu/%zu name=%s", i + 1, scene->meshes.count, Cstr(mesh->name));

		// Attach to parent in scene graph (create parent node if missing)
		sMesh parentMesh = rootMesh;
		if (ufbx_node* parent = node->parent)
		{
			auto it = meshMap.find(parent->element_id);
			if (it != meshMap.end())
				parentMesh = it->second;
			else
			{
				parentMesh = CreateNodeMesh(parent);
				meshMap[parent->element_id] = parentMesh;

				sMesh grandParentMesh = rootMesh;
				if (ufbx_node* gp = parent->parent)
				{
					auto git = meshMap.find(gp->element_id);
					if (git != meshMap.end()) grandParentMesh = git->second;
				}
				grandParentMesh->AddChild(parentMesh);
			}
		}
		parentMesh->AddChild(mesh);

		// propagate bounds up the hierarchy
		sMesh cur = mesh;
		while (auto p = cur ? Mesh::SharedPtr(cur->parent.lock()) : nullptr)
		{
			if (cur->aabb.min.x >= cur->aabb.max.x) break;
			bx::Aabb ta = TransformAabb(cur->aabb, cur->matrix);
			p->aabb.min = bx::min(p->aabb.min, ta.min);
			p->aabb.max = bx::max(p->aabb.max, ta.max);
			if (p->type & ObjectType_Group)
				for (auto& g : p->groups) g.aabb = p->aabb;
			cur = p;
		}
	}

	for (const auto& child : rootMesh->children)
	{
		const auto sp = child->parent.lock();
		ui::Log("LoadFbx: child %s: parent=%s groups=%zu children=%zu", Cstr(child->name), sp ? Cstr(sp->name) : "none", Mesh::SharedPtr(child)->groups.size(), child->children.size());
	}

	ufbx_free_scene(scene);

	return (rootMesh->children.size() == 1)
	    ? Mesh::SharedPtr(rootMesh->children[0])
	    : rootMesh;
}
