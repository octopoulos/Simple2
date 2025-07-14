// physics.cpp
// @author octopoulos
// @version 2025-07-06

#include "stdafx.h"
#include "physics.h"

PhysicsWorld::PhysicsWorld()
{
	collisionConfig = new btDefaultCollisionConfiguration();
	dispatcher      = new btCollisionDispatcher(collisionConfig);
	broadphase      = new btDbvtBroadphase();
	solver          = new btSequentialImpulseConstraintSolver();
	dynamicsWorld   = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfig);
	dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

	cubeShape      = new btBoxShape(btVector3(1, 1, 1));
	btScalar  mass = 1.0f;
	btVector3 inertia(0, 0, 0);
	cubeShape->calculateLocalInertia(mass, inertia);
	CreateCubeRigidBody(mass, inertia);

	floorShape                        = new btBoxShape(btVector3(10, 0.5, 10));
	btDefaultMotionState* floorMotion = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -2, 0)));
	auto                  floorCI     = btRigidBody::btRigidBodyConstructionInfo(0, floorMotion, floorShape, btVector3(0, 0, 0));
	floorRigidBody                    = new btRigidBody(floorCI);
	dynamicsWorld->addRigidBody(floorRigidBody);

	// {
	// 	modelShape     = new btBoxShape(btVector3(1, 1, 1));
	// 	btScalar  mass = 1.0f;
	// 	btVector3 inertia(0, 0, 0);
	// 	modelShape->calculateLocalInertia(mass, inertia);
	// 	CreateModelRigidBody(mass, inertia);
	// }
}

PhysicsWorld::~PhysicsWorld()
{
	delete dynamicsWorld;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete collisionConfig;
	delete cubeShape;
	delete floorShape;
	delete modelShape;
	delete cubeRigidBody->getMotionState();
	delete cubeRigidBody;
	delete floorRigidBody->getMotionState();
	delete floorRigidBody;
	// delete modelRigidBody->getMotionState();
	// delete modelRigidBody;
}

void PhysicsWorld::CreateCubeRigidBody(btScalar mass, btVector3 inertia)
{
	std::random_device               rd;
	std::mt19937                     gen(rd());
	std::uniform_real_distribution<> distrib(0.0, 2.0 * std::numbers::pi);
	btQuaternion                     randRot(distrib(gen), distrib(gen), distrib(gen), 1.0);
	randRot.normalize();
	btDefaultMotionState* motion = new btDefaultMotionState(btTransform(randRot, btVector3(0, 5, 0)));
	auto                  ci     = btRigidBody::btRigidBodyConstructionInfo(mass, motion, cubeShape, inertia);
	btRigidBody*          rb     = new btRigidBody(ci);
	dynamicsWorld->addRigidBody(rb);
	if (cubeRigidBody)
	{
		delete cubeRigidBody->getMotionState();
		delete cubeRigidBody;
	}
	cubeRigidBody = rb;
}

void PhysicsWorld::CreateModelRigidBody(btScalar mass, btVector3 inertia)
{
	std::random_device               rd;
	std::mt19937                     gen(rd());
	std::uniform_real_distribution<> distrib(0.0, 2.0 * std::numbers::pi);
	btQuaternion                     randRot(distrib(gen), distrib(gen), distrib(gen), 1.0);
	randRot.normalize();
	btDefaultMotionState* motion = new btDefaultMotionState(btTransform(randRot, btVector3(0, 5, 0)));
	auto                  ci     = btRigidBody::btRigidBodyConstructionInfo(mass, motion, modelShape, inertia);
	btRigidBody*          rb     = new btRigidBody(ci);
	dynamicsWorld->addRigidBody(rb);
	if (modelRigidBody)
	{
		delete modelRigidBody->getMotionState();
		delete modelRigidBody;
	}
	modelRigidBody = rb;
	modelRigidBody->activate(true);
}

void PhysicsWorld::ResetCube()
{
	dynamicsWorld->removeRigidBody(cubeRigidBody);
	btScalar  mass = 1.0f;
	btVector3 inertia(0, 0, 0);
	cubeShape->calculateLocalInertia(mass, inertia);
	CreateCubeRigidBody(mass, inertia);
	cubeRigidBody->activate(true);
}
