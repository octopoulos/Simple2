// physics.h
// @author octopoulos
// @version 2025-07-03

#pragma once

class PhysicsWorld
{
public:
	void CreateCubeRigidBody(btScalar mass, btVector3 inertia);
	void CreateModelRigidBody(btScalar mass, btVector3 inertia);

public:
	btBroadphaseInterface*               broadphase      = nullptr;
	btDefaultCollisionConfiguration*     collisionConfig = nullptr;
	btCollisionShape*                    cubeShape       = nullptr;
	btRigidBody*                         cubeRigidBody   = nullptr;
	btCollisionDispatcher*               dispatcher      = nullptr;
	btDiscreteDynamicsWorld*             dynamicsWorld   = nullptr;
	btCollisionShape*                    floorShape      = nullptr;
	btRigidBody*                         floorRigidBody  = nullptr;
	btCollisionShape*                    modelShape      = nullptr;
	btRigidBody*                         modelRigidBody  = nullptr;
	btSequentialImpulseConstraintSolver* solver          = nullptr;

	PhysicsWorld();
	~PhysicsWorld();

	void ResetCube();
};
