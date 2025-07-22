// PhysicsWorld.cpp
// @author octopoulos
// @version 2025-07-18

#include "stdafx.h"
#include "PhysicsWorld.h"

constexpr int MAX_SIMULATION_STEPS = 10;

PhysicsWorld::PhysicsWorld()
{
	config     = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(config);
	broadphase = new btDbvtBroadphase();
	solver     = new btSequentialImpulseConstraintSolver();

	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, config);
	world->setGravity(btVector3(0, -9.81f, 0));
}

PhysicsWorld::~PhysicsWorld()
{
	delete world;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete config;
}

/**
 * Run a simulation with a delta time step
 */
void PhysicsWorld::StepSimulation(float delta)
{
	if (world) world->stepSimulation(delta, MAX_SIMULATION_STEPS);
}
