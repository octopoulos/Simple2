// Body.h
// @author octopoulos
// @version 2025-07-18

#pragma once

#include "PhysicsWorld.h"

enum ShapeTypes_
{
	ShapeType_None         = 0,  ///< btEmptyShape
	ShapeType_Box          = 1,  ///< btBox2dShape
	ShapeType_Box2d        = 2,  ///< btBox2dShape
	ShapeType_Capsule      = 3,  ///< btCapsuleShape
	ShapeType_Compound     = 4,  ///< btCompoundShape
	ShapeType_Cone         = 5,  ///< btConeShape
	ShapeType_Convex2d     = 6,  ///< btConvex2dShape
	ShapeType_ConvexHull   = 7,  ///< btConvexHullShape
	ShapeType_Cylinder     = 8,  ///< btCylinderShape
	ShapeType_Heightfield  = 9,  ///< btHeightfieldTerrainShape
	ShapeType_Sphere       = 10, ///< btSphereShape
	ShapeType_StaticPlane  = 11, ///< btStaticPlaneShape
	ShapeType_TriangleMesh = 12, ///< btBvhTriangleMeshShape
};

class Body
{
public:
	btRigidBody*             body    = nullptr; ///< rigid body
	PhysicsWorld*            physics = nullptr; ///< physics reference
	btCollisionShape*        shape   = nullptr; ///< shape
	btDiscreteDynamicsWorld* world   = nullptr; ///< physical world

	btVector3 inertia = { 0.0f, 0.0f, 0.0f };
	float     mass    = 0.0f;

public:
	Body(PhysicsWorld* physics)
	    : physics(physics)
	{
		world = physics ? physics->GetWorld() : nullptr;
	}

	~Body() { Destroy(); }

	void CreateBody(float _mass, const btVector3& pos, const btQuaternion& quat);
	void CreateShape(int shapeType, const btVector3& dims);
	void Destroy();
	void DestroyBody();
	void DestroyShape();
};

using uBody = std::unique_ptr<Body>;
