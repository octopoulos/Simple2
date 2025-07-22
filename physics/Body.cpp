// Body.cpp
// @author octopoulos
// @version 2025-07-18

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
	if (world) world->addRigidBody(body);
}

void Body::CreateShape(int shapeType, const btVector4& dims, std::vector<btVector3>* vertices, std::vector<int>* indices)
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
		shape = new btConvex2dShape(nullptr);
		break;
	case ShapeType_ConvexHull:
		shape = new btConvexHullShape();
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
	{
		auto meshInterface = nullptr; // new btTriangleIndexVertexArray(...);
		shape              = new btBvhTriangleMeshShape(meshInterface, true, true);
		break;
	}
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
