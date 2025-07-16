// Body.h
// @author octopoulos
// @version 2025-07-12

#pragma once

#include "PhysicsWorld.h"

class Body
{
private:
	btRigidBody*             body    = nullptr; ///< rigid body
	PhysicsWorld*            physics = nullptr; ///< physics reference
	btCollisionShape*        shape   = nullptr; ///< shape
	btDiscreteDynamicsWorld* world   = nullptr; ///< physical world

	btVector3 inertia = { 1.0f, 1.0f, 1.0f };
	float     mass    = 1.0f;

public:
	Body(PhysicsWorld* physics)
	    : physics(physics)
	{
		world = physics->GetWorld();
	}

	~Body() { Destroy(); }

	void CreateBox(float wx, float wy, float wz);
	void Destroy();
	void DestroyBody();
	void DestroyShape();
};
