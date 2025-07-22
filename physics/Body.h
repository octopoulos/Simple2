// Body.h
// @author octopoulos
// @version 2025-07-17

#pragma once

#include "PhysicsWorld.h"

class Body
{
public:
	btRigidBody*             body    = nullptr; ///< rigid body
	PhysicsWorld*            physics = nullptr; ///< physics reference
	btCollisionShape*        shape   = nullptr; ///< shape
	btDiscreteDynamicsWorld* world   = nullptr; ///< physical world

	btVector3 inertia = { 0.0f, 0.0f, 0.0f };
	float     mass    = 1.0f;

public:
	Body(PhysicsWorld* physics)
	    : physics(physics)
	{
		world = physics ? physics->GetWorld() : nullptr;
	}

	~Body() { Destroy(); }

	void CreateBox(float wx, float wy, float wz);
	void Destroy();
	void DestroyBody();
	void DestroyShape();
};
