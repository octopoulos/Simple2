// Body.h
// @author octopoulos
// @version 2025-07-30

#pragma once

#include "physics/PhysicsWorld.h"

enum ShapeTypes_
{
	ShapeType_None         = 0,  ///< btEmptyShape
	ShapeType_Box          = 1,  ///< btBox2dShape
	ShapeType_Box2d        = 2,  ///< btBox2dShape (default: AABB)
	ShapeType_BoxObb       = 3,  ///< btBox2dShape (OBB)
	ShapeType_Capsule      = 4,  ///< btCapsuleShape
	ShapeType_Compound     = 5,  ///< btCompoundShape
	ShapeType_Cone         = 6,  ///< btConeShape
	ShapeType_Convex2d     = 7,  ///< btConvex2dShape
	ShapeType_ConvexHull   = 8,  ///< btConvexHullShape
	ShapeType_Cylinder     = 9,  ///< btCylinderShape
	ShapeType_Plane        = 10, ///< btStaticPlaneShape
	ShapeType_Sphere       = 11, ///< btSphereShape (default bounding)
	ShapeType_Terrain      = 12, ///< btHeightfieldTerrainShape
	ShapeType_TriangleMesh = 13, ///< btBvhTriangleMeshShape
	//
	ShapeType_Count,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BODY
///////

class Mesh;

class Body
{
public:
	btRigidBody*             body    = nullptr; ///< rigid body
	PhysicsWorld*            physics = nullptr; ///< physics reference
	btCollisionShape*        shape   = nullptr; ///< shape
	btDiscreteDynamicsWorld* world   = nullptr; ///< physical world

	btVector3 inertia = { 0.0f, 0.0f, 0.0f }; ///
	float     mass    = 0.0f;                 ///

public:
	Body(PhysicsWorld* physics)
	    : physics(physics)
	{
		world = physics ? physics->GetWorld() : nullptr;
	}

	~Body() { Destroy(); }

	/// Create a body after a shape has been created
	void CreateBody(float _mass, const btVector3& pos, const btQuaternion& quat);

	/// Create a collision shape, before the body
	void CreateShape(int type, Mesh* mesh = nullptr, const btVector4& dims = {});

	/// Destroy shape then body
	void Destroy();

	/// Destroy body + resets the pointer
	void DestroyBody();

	/// Destroy shape + resets the pointer
	void DestroyShape();
};

using uBody = std::unique_ptr<Body>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Find the most appropriate body shape for a given geometry type
int GeometryShape(int geometryType, bool hasMass = true, int detail = 0);
