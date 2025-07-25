// Body.cpp
// @author octopoulos
// @version 2025-07-21

#include "stdafx.h"
#include "Body.h"

#include "engine/Mesh.h"

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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

inline btVector3 BxToBullet(const bx::Vec3& vec) { return btVector3(vec.x, vec.y, vec.z); }
inline btVector3 GlmToBullet(const glm::vec3& vec) { return btVector3(vec.x, vec.y, vec.z); }

/// Helper function to compute AABB-based radius and height for Capsule, Cone, Cylinder
/// @returns {dims, center}
static std::pair<btVector3, btVector3> ComputeAabbRadiusHeight(const Group& group, const btVector3& scale, int type)
{
	const auto  half = scale * 0.5f;
	const auto& max  = BxToBullet(group.m_aabb.max) * half;
	const auto& min  = BxToBullet(group.m_aabb.min) * half;
	const auto  dims = max - min;

	float       height = dims.y();
	const float radius = std::max(dims.x(), dims.z());
	if (type != ShapeType_Cylinder)
	{
		height *= 2.0f;
		if (type == ShapeType_Capsule)
		{
			if (height < 2 * radius)
				height = 0.0f;
			else
				height -= 2 * radius;
		}
	}

	return {
		{ radius, height, radius },
		max + min
	};
}

/// Helper function to compute AABB-based vector dimensions for Box, Box2d
/// @returns {dims, center}
static std::pair<btVector3, btVector3> ComputeAabbVectorDims(const Group& group, const btVector3& scale)
{
	const auto  half = scale * 0.5f;
	const auto& max  = BxToBullet(group.m_aabb.max) * half;
	const auto& min  = BxToBullet(group.m_aabb.min) * half;
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

void Body::CreateShape(int type, Mesh* mesh, const btVector4& dims)
{
	// 1) compound rules:
	// - if multiple groups or center is not (0, 0, 0) => use a compound
	const int  numGroup = mesh ? TO_INT(mesh->groups.size()) : 0;
	const auto center   = (numGroup == 1) ? mesh->groups[0].m_sphere.center : bx::Vec3(0.0f);
	const bool noOffset = shapesNoOffsets.contains(type);
	const bool centered = noOffset ? true : std::fabsf(center.x) < 0.001f && std::fabsf(center.y) < 0.001f && std::fabsf(center.z) < 0.001f;
	auto       compound = (!noOffset && (numGroup > 1 || !centered)) ? new btCompoundShape(true, numGroup) : nullptr;

	// 2) create shape
	DestroyShape();
	switch (type)
	{
	case ShapeType_Box:
		if (dims.x() > 0.0f && dims.y() > 0.0f && dims.z() > 0.0f)
			shape = new btBoxShape(dims);
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
		else ui::LogError("CreateShape/ShapeType_Box: invalid dims / mesh");
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
		else ui::LogError("CreateShape/ShapeType_Box2d: invalid dims / mesh");
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
					transform.setFromOpenGLMatrix(group.m_obb.mtx);
					compound->addChildShape(transform, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_BoxObb: invalid mesh");
		break;
	case ShapeType_Capsule:
		if (dims.x() > 0.0f && dims.y() > 0.0f)
			shape = new btCapsuleShape(dims.x(), dims.y());
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
		else ui::LogError("CreateShape/ShapeType_Capsule: invalid dims / mesh");
		break;
	case ShapeType_Cone:
		if (dims.x() > 0.0f && dims.y() > 0.0f)
			shape = new btConeShape(dims.x(), dims.y());
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
		else ui::LogError("CreateShape/ShapeType_Cone: invalid dims / mesh");
		break;
	case ShapeType_Convex2d:
		if (numGroup > 0)
		{
			auto hull = new btConvexHullShape();
			for (const auto& group : mesh->groups)
			{
				const float*   data   = reinterpret_cast<const float*>(group.m_vertices);
				const uint16_t stride = mesh->layout.getStride();

				for (uint32_t i = 0; i < group.m_numVertices; ++i)
				{
					const float* v = data + i * (stride / sizeof(float));
					hull->addPoint(btVector3(v[0], v[1], v[2]));
				}
			}
			shape = new btConvex2dShape(hull);
		}
		else ui::LogError("CreateShape/ShapeType_Convex2d: invalid mesh");
		break;
	case ShapeType_ConvexHull:
		if (numGroup > 0)
		{
			auto            hull   = new btConvexHullShape();
			const btVector3 scale  = GlmToBullet(mesh->scale);
			const uint16_t  stride = mesh->layout.getStride();

			for (const auto& group : mesh->groups)
			{
				const float* data = reinterpret_cast<const float*>(group.m_vertices);
				for (uint32_t i = 0; i < group.m_numVertices; ++i)
				{
					const float* v = data + i * (stride / sizeof(float));
					hull->addPoint(btVector3(v[0], v[1], v[2]) * scale, false);
				}
			}

			hull->recalcLocalAabb();
			hull->initializePolyhedralFeatures();
			shape = hull;
		}
		else ui::LogError("CreateShape/ShapeType_ConvexHull: invalid mesh");
		break;
	case ShapeType_Cylinder:
		if (dims.x() > 0.0f && dims.y() > 0.0f && dims.z() > 0.0f)
			shape = new btCylinderShape(dims);
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
		else ui::LogError("CreateShape/ShapeType_Cylinder: invalid dims / mesh");
		break;
	case ShapeType_Plane:
		if (dims.x() != 0.0f || dims.y() != 0.0f || dims.z() != 0.0f)
		{
			shape = new btStaticPlaneShape(dims, dims.w());
			//shape->setLocalScaling(dims);
			shape->setMargin(0.01f);
		}
		else ui::LogError("CreateShape/ShapeType_Plane: invalid dims");
		break;
	case ShapeType_Sphere:
		if (dims.x() > 0.0f)
			shape = new btSphereShape(dims.x());
		else if (numGroup > 0)
		{
			for (const auto& group : mesh->groups)
			{
				shape = new btSphereShape(group.m_sphere.radius * mesh->scale.x);
				if (compound)
				{
					const auto goffset = btTransform(btQuaternion::getIdentity(), BxToBullet(group.m_sphere.center));
					compound->addChildShape(goffset, shape);
				}
			}
		}
		else ui::LogError("CreateShape/ShapeType_Sphere: invalid dims / mesh");
		break;
	case ShapeType_Terrain:
		shape = new btHeightfieldTerrainShape(1, 1, (const float*)nullptr, 0.0f, 0.0f, 1, false);
		break;
	case ShapeType_TriangleMesh:
		if (numGroup > 0)
		{
			auto            trimesh = new btTriangleMesh();
			const btVector3 scale   = GlmToBullet(mesh->scale);
			const uint16_t  stride  = mesh->layout.getStride();

			for (const auto& group : mesh->groups)
			{
				const float* data = reinterpret_cast<const float*>(group.m_vertices);
				for (uint32_t i = 0; i + 2 < group.m_numIndices; i += 3)
				{
					const uint16_t i0 = group.m_indices[i + 0];
					const uint16_t i1 = group.m_indices[i + 1];
					const uint16_t i2 = group.m_indices[i + 2];

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
		else ui::LogError("CreateShape/ShapeType_TriangleMesh: invalid mesh");
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
