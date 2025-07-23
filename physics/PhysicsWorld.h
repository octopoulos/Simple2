// PhysicsWorld.h
// @author octopoulos
// @version 2025-07-19

#pragma once

#include <btBulletDynamicsCommon.h>
#include <bgfxh/bgfxh.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BulletDebugDraw
//////////////////

class BulletDebugDraw : public btIDebugDraw
{
public:
	int          _debugMode       = 0;  ///
	float        bgfxModelMtx[16] = {}; ///
	bgfx::ViewId viewId           = 0;  ///

	BulletDebugDraw()
	    : btIDebugDraw()
	{
	}

	virtual void draw3dText(const btVector3& location, const char* textString) override {}

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;

	virtual int getDebugMode() const override { return _debugMode; }

	virtual void reportErrorWarning(const char* warningString) override {}

	virtual void setDebugMode(int debugMode) override { _debugMode = debugMode; }
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PhysicsWorld
///////////////

class PhysicsWorld
{
private:
	std::unique_ptr<BulletDebugDraw>     debugDraw;                    ///< debugging
	int                                  maxSimulationSteps = 10;      ///< maximum number of simulation steps per frame
	btBroadphaseInterface*               broadphase         = nullptr; ///
	btDefaultCollisionConfiguration*     config             = nullptr; ///
	btCollisionDispatcher*               dispatcher         = nullptr; ///
	btSequentialImpulseConstraintSolver* solver             = nullptr; ///
	btDiscreteDynamicsWorld*             world              = nullptr; ///< dynamic world

public:
	PhysicsWorld();
	~PhysicsWorld();

	void DrawDebug();

	/// Run a simulation with a delta time step
	void StepSimulation(float delta);

	btDiscreteDynamicsWorld* GetWorld() const { return world; }
};
