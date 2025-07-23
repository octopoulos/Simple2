// Body.cpp
// @author octopoulos
// @version 2025-07-19

#include "stdafx.h"
#include "Body.h"

#include <BulletCollision/CollisionShapes/btBox2dShape.h>
#include <BulletCollision/CollisionShapes/btConvex2dShape.h>
#include <BulletCollision/CollisionShapes/btHeightfieldTerrainShape.h>

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
	body->setFriction(0.4f);
	body->setRollingFriction(0.015f);
	body->setSpinningFriction(0.02f);

	if (world) world->addRigidBody(body);
}

void Body::CreateShape(int shapeType, const btVector4& dims, Mesh* mesh)
{
	DestroyShape();
	switch (shapeType)
	{
	case ShapeType_Box:
		shape = new btBoxShape(dims);
		break;
	case ShapeType_Box2d:
		shape = new btBox2dShape(dims);
		break;
	case ShapeType_Capsule:
		shape = new btCapsuleShape(dims.x(), dims.y());
		break;
	case ShapeType_Compound:
		shape = new btCompoundShape();
		break;
	case ShapeType_Cone:
		shape = new btConeShape(dims.x(), dims.y());
		break;
	case ShapeType_Convex2d:
		if (mesh)
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
		break;
	case ShapeType_ConvexHull:
		if (mesh)
		{
			auto           hull   = new btConvexHullShape();
			const uint16_t stride = mesh->layout.getStride();

			for (const auto& group : mesh->groups)
			{
				const float* data = reinterpret_cast<const float*>(group.m_vertices);
				for (uint32_t i = 0; i < group.m_numVertices; ++i)
				{
					const float* v = data + i * (stride / sizeof(float));
					hull->addPoint(btVector3(v[0], v[1], v[2]), false);
				}
			}

			hull->recalcLocalAabb();
			hull->initializePolyhedralFeatures();
			shape = hull;
		}
		break;
	case ShapeType_Cylinder:
		shape = new btCylinderShape(dims);
		break;
	case ShapeType_Plane:
	{
		shape = new btStaticPlaneShape(dims, dims.w());
		shape->setLocalScaling(dims);
		shape->setMargin(0.01f);
		break;
	}
	case ShapeType_Sphere:
		shape = new btSphereShape(dims.x());
		break;
	case ShapeType_Terrain:
		shape = new btHeightfieldTerrainShape(1, 1, (const float*)nullptr, 0.0f, 0.0f, 1, false);
		break;
	case ShapeType_TriangleMesh:
		if (mesh)
		{
			auto           trimesh = new btTriangleMesh();
			const uint16_t stride  = mesh->layout.getStride();

			for (const auto& group : mesh->groups)
			{
				const float* data = reinterpret_cast<const float*>(group.m_vertices);
				for (uint32_t i = 0; i + 2 < group.m_numIndices; i += 3)
				{
					uint16_t i0 = group.m_indices[i + 0];
					uint16_t i1 = group.m_indices[i + 1];
					uint16_t i2 = group.m_indices[i + 2];

					const float* v0 = data + i0 * (stride / sizeof(float));
					const float* v1 = data + i1 * (stride / sizeof(float));
					const float* v2 = data + i2 * (stride / sizeof(float));

					trimesh->addTriangle(
					    btVector3(v0[0], v0[1], v0[2]),
					    btVector3(v1[0], v1[1], v1[2]),
					    btVector3(v2[0], v2[1], v2[2]),
					    true
					);
				}
			}

			shape = new btBvhTriangleMeshShape(trimesh, true, true);
		}
		break;
	default: ui::LogError("CreateShape: Unknown type: {}", shapeType);
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
