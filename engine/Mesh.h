// Mesh.h
// @author octopoulos
// @version 2025-07-11

#pragma once

#include <bx/bounds.h>

struct Primitive
{
	uint32_t m_startIndex;
	uint32_t m_numIndices;
	uint32_t m_startVertex;
	uint32_t m_numVertices;

	bx::Sphere m_sphere;
	bx::Aabb   m_aabb;
	bx::Obb    m_obb;
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

	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle  m_ibh;
	uint16_t                 m_numVertices;
	uint8_t*                 m_vertices;
	uint32_t                 m_numIndices;
	uint16_t*                m_indices;
	bx::Sphere               m_sphere;
	bx::Aabb                 m_aabb;
	bx::Obb                  m_obb;
	std::vector<Primitive>   m_prims;
};

struct MeshState
{
	struct Texture
	{
		uint32_t            m_flags;
		bgfx::UniformHandle m_sampler;
		bgfx::TextureHandle m_texture;
		uint8_t             m_stage;
	};

	Texture             m_textures[4];
	uint64_t            m_state;
	bgfx::ProgramHandle m_program;
	uint8_t             m_numTextures;
	uint16_t            m_viewId;
};

class Mesh : public Object3d
{
public:
	btRigidBody*              body     = nullptr; //
	std::shared_ptr<Geometry> geometry = nullptr; //
	std::shared_ptr<Material> material = nullptr; //
	bgfx::ProgramHandle       program  = {};      //
	std::vector<Group>        groups   = {};      //
	bgfx::VertexLayout        layout   = {};      //

	Mesh() = default;

	Mesh(std::shared_ptr<Geometry> geometry, std::shared_ptr<Material> material)
	    : geometry(std::move(geometry))
	    , material(std::move(material))
	{
	}

	~Mesh() { Destroy(); }

	void Destroy();
	void Load(bx::ReaderSeekerI* reader, bool ramcopy);
	void Render(uint8_t viewId) override;
	void Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const;
	void Submit(const MeshState* const* state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const;
};

using sMesh = std::shared_ptr<Mesh>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

sMesh      MeshLoad(const bx::FilePath& filePath, bool ramcopy = false);
MeshState* MeshStateCreate();
void       MeshStateDestroy(MeshState* meshState);
