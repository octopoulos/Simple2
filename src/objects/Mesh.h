// Mesh.h
// @author octopoulos
// @version 2025-09-08

#pragma once

#include "core/Camera.h"
#include "geometries/Geometry.h"
#include "materials/Material.h"
#include "objects/Object3d.h"
#include "physics/Body.h"

#include <bx/bounds.h>

enum MeshLoads_ : int
{
	MeshLoad_None  = 0, ///< created manually, ex: geometry + shaders + textures
	MeshLoad_Basic = 1, ///< LoadModel
	MeshLoad_Full  = 2, ///< LoadModelFull
};

struct Primitive
{
	uint32_t   startIndex  = 0;  ///
	uint32_t   numIndices  = 0;  ///
	uint32_t   startVertex = 0;  ///
	uint32_t   numVertices = 0;  ///
	bx::Aabb   aabb        = {}; ///
	bx::Obb    obb         = {}; ///
	bx::Sphere sphere      = {}; ///
};

struct Group
{
	Group() { Reset(); }

	void Reset()
	{
		ibh.idx     = bgfx::kInvalidHandle;
		indices     = nullptr;
		numIndices  = 0;
		numVertices = 0;
		vbh.idx     = bgfx::kInvalidHandle;
		vertices    = nullptr;
		prims.clear();
	}

	bx::Aabb                 aabb        = {};                  ///
	bgfx::IndexBufferHandle  ibh         = BGFX_INVALID_HANDLE; ///
	uint32_t*                indices     = nullptr;             ///
	uint32_t                 numIndices  = 0;                   ///
	uint32_t                 numVertices = 0;                   ///
	bx::Obb                  obb         = {};                  ///
	std::vector<Primitive>   prims       = {};                  ///
	bx::Sphere               sphere      = {};                  ///
	bgfx::VertexBufferHandle vbh         = BGFX_INVALID_HANDLE; ///
	uint8_t*                 vertices    = nullptr;             ///
	sMaterial                material    = nullptr;             ///
};

struct MeshState
{
	struct Texture
	{
		uint32_t            flags;   ///
		bgfx::UniformHandle sampler; ///
		uint8_t             stage;   ///
		bgfx::TextureHandle texture; ///
	};

	bgfx::ProgramHandle program;     ///
	uint8_t             numTextures; ///
	uint64_t            state;       ///
	Texture             textures[4]; ///
	uint16_t            viewId;      ///
};

using sMesh = std::shared_ptr<class Mesh>;

class Mesh : public Object3d
{
public:
	uBody                     body      = {};            ///< one body for the whole mesh
	std::shared_ptr<Geometry> geometry  = nullptr;       ///
	std::vector<Group>        groups    = {};            ///< groups of vertices
	bgfx::VertexLayout        layout    = {};            ///
	int                       load      = MeshLoad_None; ///< how the model was loaded (for open/save scene)
	sMaterial                 material  = nullptr;       ///< current material (might be "cursor")
	sMaterial                 material0 = nullptr;       ///< original material
	std::string               modelName = "";            ///< model name (part of filename)

	Mesh(std::string_view name, int typeFlag = 0)
	    : Object3d(name, ObjectType_Mesh | typeFlag)
	{
	}

	Mesh(std::string_view name, std::shared_ptr<Geometry> geometry, sMaterial material)
	    : Object3d(name, ObjectType_Mesh)
	    , geometry(std::move(geometry))
	    , material(std::move(material))
	{
	}

	virtual ~Mesh() override { Destroy(); }

	/// Activate/deactivate physics
	void ActivatePhysics(bool activate);

	/// Make an instanced copy of itself => faster loading
	sMesh CloneInstance(std::string_view cloneName);

	/// Specific controls
	virtual void Controls(const sCamera& camera, int modifier, const bool* downs, bool* ignores, const bool* keys);

	/// Utility to create a shape then a body
	void CreateShapeBody(PhysicsWorld* physics, int shapeType, float mass = 0.0f, const btVector4& newDims = { 0.0f, 0.0f, 0.0f, 0.0f });

	/// Delete all groups including indices + vertices
	void Destroy();

	/// Render the mesh, if geometry & material program exist, or if program is set
	/// - if a group => render the children only
	virtual void Render(uint8_t viewId, int renderFlags) override;

	/// Serialize for JSON output
	virtual int Serialize(fmt::memory_buffer& outString, int depth, int bounds = 3, bool addChildren = true) const override;

	/// Convert matrixTransform to bullet3:transform
	void SetBodyTransform();

	/// Get an Object3d as a Mesh
	static sMesh SharedPtr(const sObject3d& object, int type = ObjectType_Mesh)
	{
		return (object && (object->type & type)) ? std::static_pointer_cast<Mesh>(object) : nullptr;
	}

	/// Show settings in ImGui
	/// @param show: ShowObjects_
	virtual void ShowSettings(bool isPopup, int show) override;

	/// Show info table in ImGui
	virtual void ShowTable() const override;

	/// Submit for render pass
	void Submit(uint16_t id, bgfx::ProgramHandle program, const float* mtx, uint64_t state) const;

	/// Submit for multi passes
	void Submit(const MeshState* const* state, uint8_t numPasses, const float* mtx, uint16_t numMatrices) const;

	/// Synchronize physics transform
	/// - if a group => update the children only
	virtual int SynchronizePhysics() override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

MeshState* MeshStateCreate();
void       MeshStateDestroy(MeshState* meshState);
