// Mesh.h
// @author octopoulos
// @version 2025-07-26

#pragma once

#include "geometries/Geometry.h"
#include "materials/Material.h"
#include "objects/Object3d.h"
#include "physics/Body.h"

#include <bx/bounds.h>

struct Primitive
{
	uint32_t m_startIndex;  ///
	uint32_t m_numIndices;  ///
	uint32_t m_startVertex; ///
	uint32_t m_numVertices; ///

	bx::Sphere m_sphere; ///
	bx::Aabb   m_aabb;   ///
	bx::Obb    m_obb;    ///
};

struct Group
{
	Group() { Reset(); }

	void Reset()
	{
		m_ibh.idx     = bgfx::kInvalidHandle;
		m_indices     = nullptr;
		m_numIndices  = 0;
		m_numVertices = 0;
		m_vbh.idx     = bgfx::kInvalidHandle;
		m_vertices    = nullptr;
		m_prims.clear();
	}

	bx::Aabb                 m_aabb;        ///
	bgfx::IndexBufferHandle  m_ibh;         ///
	uint16_t*                m_indices;     ///
	uint32_t                 m_numIndices;  ///
	uint16_t                 m_numVertices; ///
	bx::Obb                  m_obb;         ///
	std::vector<Primitive>   m_prims;       ///
	bx::Sphere               m_sphere;      ///
	bgfx::VertexBufferHandle m_vbh;         ///
	uint8_t*                 m_vertices;    ///
};

struct MeshState
{
	struct Texture
	{
		uint32_t            m_flags;   ///
		bgfx::UniformHandle m_sampler; ///
		uint8_t             m_stage;   ///
		bgfx::TextureHandle m_texture; ///
	};

	bgfx::ProgramHandle m_program;     ///
	uint8_t             m_numTextures; ///
	uint64_t            m_state;       ///
	Texture             m_textures[4]; ///
	uint16_t            m_viewId;      ///
};

class Mesh : public Object3d
{
public:
	std::vector<uBody>        bodies   = {};      ///< one physical body per group
	std::shared_ptr<Geometry> geometry = nullptr; ///
	std::vector<Group>        groups   = {};      ///< groups of vertices
	bgfx::VertexLayout        layout   = {};      ///
	std::shared_ptr<Material> material = nullptr; ///
	bgfx::ProgramHandle       program  = {};      ///
	uint64_t                  state    = 0;       ///< if !=0: override default state

	Mesh()
	{
		type = ObjectType_Mesh;
	}

	Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material)
	    : geometry(std::move(geometry))
	    , material(std::move(material))
	{
		type = ObjectType_Mesh;
	}

	~Mesh() { Destroy(); }

	/// Utility to create a shape then a body
	void CreateShapeBody(PhysicsWorld* physics, int shapeType, float mass = 0.0f, const btVector4& dims = { 0.0f, 0.0f, 0.0f, 0.0f });

	/// Delete all groups including indices + vertices
	void Destroy();

	/// Load a mesh
	/// @param ramcopy: populate indices and vertices in groups
	void Load(bx::ReaderSeekerI* reader, bool ramcopy);

	/// Render the mesh, if geometry & material program exist, or if program is set
	/// - if a group => render the children only
	virtual void Render(uint8_t viewId, int renderFlags) override;

	/// Submit for render pass
	void Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const;

	/// Submit for multi passes
	void Submit(const MeshState* const* state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const;

	/// Synchronize physics transform
	/// - if a group => update the children only
	virtual void SynchronizePhysics() override;
};

using sMesh = std::shared_ptr<Mesh>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

sMesh      MeshLoad(const bx::FilePath& filePath, bool ramcopy = false);
MeshState* MeshStateCreate();
void       MeshStateDestroy(MeshState* meshState);
