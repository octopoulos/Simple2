// PhysicsWorld.h
// @author octopoulos
// @version 2025-07-12

#pragma once

#include <btBulletDynamicsCommon.h>

class PhysicsWorld
{
private:
	btBroadphaseInterface*               broadphase = nullptr; ///
	btDefaultCollisionConfiguration*     config     = nullptr; ///
	btCollisionDispatcher*               dispatcher = nullptr; ///
	btSequentialImpulseConstraintSolver* solver     = nullptr; ///
	btDiscreteDynamicsWorld*             world      = nullptr; ///< dynamic world

public:
	PhysicsWorld();
	~PhysicsWorld();

	/// Run a simulation with a delta time step
	void StepSimulation(float delta);

	btDiscreteDynamicsWorld* GetWorld() const { return world; }
};
