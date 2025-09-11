// Body.cpp
// @author octopoulos
// @version 2025-09-07

#include "stdafx.h"
#include "physics/Body.h"
//
#include "core/common3d.h"  // BulletToGlm, BxToBullet, GlmToBullet
#include "loaders/writer.h" // WRITE_INIT, WRITE_KEY_xxx
#include "objects/Mesh.h"   // Group, Mesh
#include "ui/ui.h"          // ui::

#include <BulletCollision/CollisionShapes/btBox2dShape.h>
#include <BulletCollision/CollisionShapes/btConvex2dShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

/// Those shapes require no offset
static const USET_INT shapesNoOffsets = {
	ShapeType_BoxObb,
	ShapeType_Convex2d,
	ShapeType_ConvexHull,
	ShapeType_Plane,
	ShapeType_TriangleMesh,
};

// clang-format off
static const UMAP_INT_STR shapeTypeNames = {
	{ ShapeType_None        , "None"         },
	{ ShapeType_Box         , "Box"          },
	{ ShapeType_Box2d       , "Box2d"        },
	{ ShapeType_BoxObb      , "BoxObb"       },
	{ ShapeType_Capsule     , "Capsule"      },
	{ ShapeType_Compound    , "Compound"     },
	{ ShapeType_Cone        , "Cone"         },
	{ ShapeType_Convex2d    , "Convex2d"     },
	{ ShapeType_ConvexHull  , "ConvexHull"   },
	{ ShapeType_Cylinder    , "Cylinder"     },
	{ ShapeType_Plane       , "Plane"        },
	{ ShapeType_Sphere      , "Sphere"       },
	{ ShapeType_Terrain     , "Terrain"      },
	{ ShapeType_TriangleMesh, "TriangleMesh" },
};
// clang-format on

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

/// Helper function to compute AABB-based radius and height for Capsule, Cone, Cylinder
/// @returns {dims, center}
static std::pair<btVector3, btVector3> ComputeAabbRadiusHeight(const Group& group, const btVector3& scale, int type)
{
	const auto  half    = scale * 0.5f;
	const auto& halfMax = BxToBullet(group.aabb.max) * half;
	const auto& halfMin = BxToBullet(group.aabb.min) * half;
	const auto  dims    = halfMax - halfMin;

	float       height = dims.y();
	const float radius = (dims.x() + dims.z()) * 0.5f;

	if (type != ShapeType_Cylinder)
	{
		height *= 2.0f;
		if (type == ShapeType_Capsule)
		{
			if (height > 2.0f * radius)
				height -= 2.0f * radius;
			else
				height = 0.0f;
		}
	}

	return {
		{ radius, height, radius },
		halfMax + halfMin
	};
}

/// Helper function to compute AABB-based vector dimensions for Box, Box2d
/// @returns {dims, center}
static std::pair<btVector3, btVector3> ComputeAabbVectorDims(const Group& group, const btVector3& scale)
{
	const auto  half = scale * 0.5f;
	const auto& max  = BxToBullet(group.aabb.max) * half;
	const auto& min  = BxToBullet(group.aabb.min) * half;
	return {
		max - min,
		max + min
	};
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BODY
///////

void Body::CreateBody(float _mass, const btVector3& pos, const btQuaternion& quat)
{
	DestroyBody();
	if (!shape)
	{
		ui::LogError("CreateBody: Shape is required");
		return;
	}

	mass = _mass;
	if (mass > 0.0f)
		shape->calculateLocalInertia(mass, inertia);
	else
		inertia = btVector3(0.0f, 0.0f, 0.0f);

	auto motion = new btDefaultMotionState(btTransform(quat, pos));
	auto ci     = btRigidBody::btRigidBodyConstructionInfo(mass, motion, shape, inertia);

	body = new btRigidBody(ci);
	//ui::Log("friction={} rollingFriction={} spinningFriction={}", body->getFriction(), body->getRollingFriction(), body->getSpinningFriction());
	body->setDamping(0.25f, 0.1f);
	body->setFriction(0.4f);
	body->setRollingFriction(0.015f);
	body->setSpinningFriction(0.02f);
	body->setSleepingThresholds(1.2f, 1.5f);

	if (world) world->addRigidBody(body);
}

void Body::CreateShape(int type, Mesh* mesh, const btVector4& newDims)
{
	shapeType = type;

	// override dims
	if (newDims.x() > 0.0f || newDims.y() > 0.0f || newDims.z() > 0.0f)
		dims = newDims;

	// 1) compound rules:
	// - if multiple groups or center is not (0, 0, 0) => use a compound
	const int  numGroup = mesh ? TO_INT(mesh->groups.size()) : 0;
	const auto center   = (numGroup == 1) ? mesh->groups[0].sphere.center : bx::Vec3(0.0f);
	const auto geometry = mesh ? mesh->geometry : nullptr;
	const bool noOffset = shapesNoOffsets.contains(type);
	const bool centered = noOffset ? true : std::fabsf(center.x) < 0.001f && std::fabsf(center.y) < 0.001f && std::fabsf(center.z) < 0.001f;
	auto       compound = (!noOffset && (numGroup > 1 || !centered)) ? new btCompoundShape(true, numGroup) : nullptr;

	// 2) create shape
	DestroyShape();
	switch (type)
	{
	case ShapeType_Box:
		ui::Log("{}: dims={} {} {}", type, dims.x(), dims.y(), dims.z());
		if (dims.x() > 0.0f && dims.y() > 0.0f && dims.z() > 0.0f)
			shape = new btBoxShape(dims);
		else if (geometry)
			shape = new btBoxShape(geometry->aabb * mesh->scale.x);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				const auto& [gdims, gcenter] = ComputeAabbVectorDims(group, GlmToBullet(mesh->scale));

				shape = new btBoxShape(gdims);
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), gcenter);
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Box: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_Box2d:
		if (dims.x() > 0.0f && dims.y() > 0.0f && dims.z() > 0.0f)
			shape = new btBox2dShape(dims);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				const auto& [gdims, gcenter] = ComputeAabbVectorDims(group, GlmToBullet(mesh->scale));

				shape = new btBox2dShape(gdims);
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), gcenter);
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Box2d: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_BoxObb:
		if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				shape = new btBoxShape(GlmToBullet(mesh->scale));
				if (compound)
				{
					btTransform transform;
					transform.setFromOpenGLMatrix(group.obb.mtx);
					compound->addChildShape(transform, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_BoxObb: Invalid mesh");
		break;
	case ShapeType_Capsule:
		if (dims.x() > 0.0f && dims.y() > 0.0f)
			shape = new btCapsuleShape(dims.x(), dims.y());
		else if (geometry)
			shape = new btCapsuleShape(geometry->dims[0] * mesh->scale.x, geometry->dims[1] * mesh->scale.x);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				const auto [gdims, gcenter] = ComputeAabbRadiusHeight(group, GlmToBullet(mesh->scale), type);

				shape = new btCapsuleShape(gdims.x(), gdims.y());
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), gcenter);
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Capsule: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_Cone:
		if (dims.x() > 0.0f && dims.y() > 0.0f)
			shape = new btConeShape(dims.x(), dims.y());
		else if (geometry)
			shape = new btConeShape(geometry->dims[0] * mesh->scale.x, geometry->dims[1] * mesh->scale.x);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				const auto [gdims, gcenter] = ComputeAabbRadiusHeight(group, GlmToBullet(mesh->scale), type);

				shape = new btConeShape(gdims.x(), gdims.y());
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), gcenter);
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Cone: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_Convex2d:
		if (numGroup > 0)
		{
			auto hull = new btConvexHullShape();
			for (const auto& group : mesh->groups)
			{
				const float*   data   = reinterpret_cast<const float*>(group.vertices);
				const uint16_t stride = mesh->layout.getStride();

				for (uint32_t i = 0; i < group.numVertices; ++i)
				{
					const float* v = data + i * (stride / sizeof(float));
					hull->addPoint(btVector3(v[0], v[1], v[2]));
				}
			}
			shape = new btConvex2dShape(hull);
		}
		else ui::LogError("CreateShape/ShapeType_Convex2d: Invalid mesh");
		break;
	case ShapeType_ConvexHull:
		if (geometry && geometry->vertices.size())
		{
			auto            hull  = new btConvexHullShape();
			const btVector3 scale = GlmToBullet(mesh->scale);

			for (const auto& vertex : geometry->vertices)
				hull->addPoint(btVector3(vertex.px, vertex.py, vertex.pz) * scale);

			hull->initializePolyhedralFeatures();
			shape = hull;
		}
		else if (numGroup > 0)
		{
			auto            hull   = new btConvexHullShape();
			const btVector3 scale  = GlmToBullet(mesh->scale);
			const uint16_t  stride = mesh->layout.getStride();

			for (const auto& group : mesh->groups)
			{
				const float* data = reinterpret_cast<const float*>(group.vertices);
				for (uint32_t i = 0; i < group.numVertices; ++i)
				{
					const float* v = data + i * (stride / sizeof(float));
					hull->addPoint(btVector3(v[0], v[1], v[2]) * scale);
				}
			}

			hull->initializePolyhedralFeatures();
			shape = hull;
		}
		else ui::LogError("CreateShape/ShapeType_ConvexHull: Invalid mesh");
		break;
	case ShapeType_Cylinder:
		if (dims.x() > 0.0f && dims.y() > 0.0f && dims.z() > 0.0f)
			shape = new btCylinderShape(dims);
		else if (geometry)
			shape = new btCylinderShape(geometry->aabb * mesh->scale.x);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				const auto& [gdims, gcenter] = ComputeAabbRadiusHeight(group, GlmToBullet(mesh->scale), type);

				shape = new btCylinderShape(gdims);
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), gcenter);
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Cylinder: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_Plane:
		if (dims.x() != 0.0f || dims.y() != 0.0f || dims.z() != 0.0f)
		{
			shape = new btStaticPlaneShape(dims, dims.w());
			shape->setMargin(0.01f);
		}
		else ui::LogError("CreateShape/ShapeType_Plane: Invalid dims");
		break;
	case ShapeType_Sphere:
		if (dims.x() > 0.0f)
			shape = new btSphereShape(dims.x());
		else if (geometry)
			shape = new btSphereShape(geometry->radius * mesh->scale.x);
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				shape = new btSphereShape(group.sphere.radius * mesh->scale.x);
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), BxToBullet(group.sphere.center));
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Sphere: Invalid dims / mesh: {} {} {} {} {}", dims.x(), dims.y(), dims.z(), !!geometry, numGroup);
		break;
	case ShapeType_Terrain:
		shape = new btHeightfieldTerrainShape(1, 1, (const float*)nullptr, 0.0f, 0.0f, 1, false);
		break;
	case ShapeType_TriangleMesh:
		if (geometry && geometry->indices.size())
		{
			const auto&     indices  = geometry->indices;
			const auto&     vertices = geometry->vertices;
			const btVector3 scale    = GlmToBullet(mesh->scale);
			auto            trimesh  = new btTriangleMesh();

			for (uint32_t i = 0, numIndex = TO_UINT32(indices.size()); i < numIndex; i += 3)
			{
				const uint16_t i0 = indices[i + 0];
				const uint16_t i1 = indices[i + 1];
				const uint16_t i2 = indices[i + 2];

				const auto& v0 = vertices[i0];
				const auto& v1 = vertices[i1];
				const auto& v2 = vertices[i2];

				trimesh->addTriangle(
					btVector3(v0.px, v0.py, v0.pz) * scale,
					btVector3(v1.px, v1.py, v1.pz) * scale,
					btVector3(v2.px, v2.py, v2.pz) * scale,
					true
				);
			}

			shape = new btBvhTriangleMeshShape(trimesh, true, true);
		}
		else if (numGroup > 0)
		{
			const btVector3 scale   = GlmToBullet(mesh->scale);
			const uint16_t  stride  = mesh->layout.getStride();
			auto            trimesh = new btTriangleMesh();

			for (const auto& group : mesh->groups)
			{
				if (!group.indices || !group.vertices)
				{
					ui::LogError("CreateShape/ShapeType_TriangleMesh: Use ramcopy=true");
					break;
				}

				const float* data = reinterpret_cast<const float*>(group.vertices);
				for (uint32_t i = 0; i + 2 < group.numIndices; i += 3)
				{
					const uint16_t i0 = group.indices[i + 0];
					const uint16_t i1 = group.indices[i + 1];
					const uint16_t i2 = group.indices[i + 2];

					const float* v0 = data + i0 * (stride / sizeof(float));
					const float* v1 = data + i1 * (stride / sizeof(float));
					const float* v2 = data + i2 * (stride / sizeof(float));

					trimesh->addTriangle(
					    btVector3(v0[0], v0[1], v0[2]) * scale,
					    btVector3(v1[0], v1[1], v1[2]) * scale,
					    btVector3(v2[0], v2[1], v2[2]) * scale,
					    true
					);
				}
			}

			shape = new btBvhTriangleMeshShape(trimesh, true, true);
		}
		else ui::LogError("CreateShape/ShapeType_TriangleMesh: Invalid mesh");
		break;
	default: ui::LogError("CreateShape: Unknown type: {}", type);
	}

	// 3) set shape to compound + possibly offset the shape
	if (compound)
	{
		if (!centered && numGroup == 1 && !compound->getNumChildShapes())
		{
			const auto offset = btTransform(btQuaternion::getIdentity(), btVector3(center.x, center.y, center.z));
			compound->addChildShape(offset, shape);
		}
		shape = compound;
	}
}

void Body::Destroy()
{
	DestroyShape();
	DestroyBody();
}

void Body::DestroyBody()
{
	if (body)
	{
		if (world) world->removeRigidBody(body);
		const auto& motionState = body->getMotionState();
		if (motionState) delete motionState;
		delete body;
		body = nullptr;
	}
}

void Body::DestroyShape()
{
	if (shape)
	{
		delete shape;
		shape = nullptr;
	}
}

int Body::Serialize(fmt::memory_buffer& outString, int depth, int bounds) const
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();

	if (const auto& vec = dims; vec.x() != 0.0f || vec.y() != 0.0f || vec.z() != 0.0f || vec.w() != 0.0f)
	{
		const auto dims = BulletToGlm(vec);
		WRITE_KEY_VEC4(dims);
	}

	WRITE_KEY_BOOL(enabled);
	WRITE_KEY_FLOAT(mass);
	WRITE_KEY_STRING2("shapeType", ShapeName(shapeType));
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Body::ShowSettings(bool isPopup, int show)
{
	int mode = 3;
	if (isPopup) mode |= 4;

	ui::AddCheckbox(mode, ".enabled", "", "Enabled", &enabled);
	ui::AddDragFloat(mode, ".mass", "Mass", &mass);
}

void Body::ShowTable()
{
	// clang-format off
	ui::ShowTable({
		{ "enabled"  , BoolString(enabled)                                    },
		{ "mass"     , std::to_string(mass)                                   },
		{ "shapeType", fmt::format("{}: {}", shapeType, ShapeName(shapeType)) },
	});
	// clang-format on
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

int GeometryShape(int geometryType, bool hasMass, int detail)
{
	// clang-format off
	switch (geometryType)
	{
	// easy cases
	case GeometryType_Box:      return ShapeType_Box;
	case GeometryType_Capsule:  return ShapeType_Capsule;
	case GeometryType_Cone:     return ShapeType_Cone;
	case GeometryType_Cylinder: return ShapeType_Cylinder;
	case GeometryType_Sphere:   return ShapeType_Sphere;

	// detail => looks like a sphere
	case GeometryType_Dodecahedron:
	case GeometryType_Icosahedron: return (detail >= 1) ? ShapeType_Sphere : ShapeType_ConvexHull;
	case GeometryType_Octahedron:  return (detail >= 2) ? ShapeType_Sphere : ShapeType_ConvexHull;
	case GeometryType_Tetrahedron: return (detail >= 4) ? ShapeType_Sphere : ShapeType_ConvexHull;

	// here, only triangle is good but if static (no mass)
	case GeometryType_Torus:     return hasMass ? ShapeType_Cylinder   : ShapeType_TriangleMesh;
	case GeometryType_TorusKnot: return hasMass ? ShapeType_ConvexHull : ShapeType_TriangleMesh;
	}
	// clang-format on

	return ShapeType_Box;
}

TEST_CASE("GeometryShape")
{
	// clang-format off
	const std::vector<std::tuple<int, bool, int, int>> vectors = {
		{ GeometryType_Box         , true , 0, ShapeType_Box          },
		{ GeometryType_Box         , false, 0, ShapeType_Box          },
		{ GeometryType_Capsule     , true , 0, ShapeType_Capsule      },
		{ GeometryType_Cone        , true , 0, ShapeType_Cone         },
		{ GeometryType_Cylinder    , true , 0, ShapeType_Cylinder     },
		{ GeometryType_Sphere      , true , 0, ShapeType_Sphere       },
		{ GeometryType_Dodecahedron, true , 0, ShapeType_ConvexHull   },
		{ GeometryType_Dodecahedron, true , 1, ShapeType_Sphere       },
		{ GeometryType_Dodecahedron, true , 4, ShapeType_Sphere       },
		{ GeometryType_Icosahedron , true , 0, ShapeType_ConvexHull   },
		{ GeometryType_Octahedron  , true , 0, ShapeType_ConvexHull   }, // 10
		{ GeometryType_Tetrahedron , true , 0, ShapeType_ConvexHull   },
		{ GeometryType_Torus       , true , 0, ShapeType_Cylinder     },
		{ GeometryType_Torus       , false, 0, ShapeType_TriangleMesh },
		{ GeometryType_TorusKnot   , true , 0, ShapeType_ConvexHull   },
		{ GeometryType_TorusKnot   , false, 0, ShapeType_TriangleMesh },
	};
	// clang-format on
	for (int i = -1; const auto& [geometryType, hasMass, detail, answer] : vectors)
	{
		SUBCASE_FMT("{}_{}_{}_{}", ++i, geometryType, hasMass, detail)
		CHECK(GeometryShape(geometryType, hasMass, detail) == answer);
	}
}

std::string ShapeName(int type)
{
	return FindDefault(shapeTypeNames, type, "???");
}

int ShapeType(std::string_view name)
{
	static UMAP_STR_INT shapeNameTypes;
	if (shapeNameTypes.empty())
	{
		for (const auto& [type, name] : shapeTypeNames)
			shapeNameTypes[name] = type;
	}

	return FindDefault(shapeNameTypes, name, GeometryType_None);
}
