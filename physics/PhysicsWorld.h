// PhysicsWorld.h
// @author octopoulos
// @version 2025-07-21

#pragma once

#include <btBulletDynamicsCommon.h>
#include <bgfxh/bgfxh.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BulletDebugDraw
//////////////////

struct PosColorVertex
{
	float    m_x, m_y, m_z;
	uint32_t m_abgr;

	static bgfx::VertexLayout ms_layout;
};

class BulletDebugDraw : public btIDebugDraw
{
public:
	float                       bgfxModelMtx[16] = {}; ///
	int                         debugMode        = 0;  ///
	std::vector<PosColorVertex> lines            = {}; ///< collect all lines
	bgfx::ViewId                viewId           = 0;  ///< view ID for rendering

	BulletDebugDraw()
	    : btIDebugDraw()
	{
	}

	virtual void draw3dText(const btVector3& location, const char* textString) override {}

	virtual void drawContactPoint(const btVector3& PointOnB, const btVector3& normalOnB, btScalar distance, int lifeTime, const btVector3& color) override {}

	virtual void drawLine(const btVector3& from, const btVector3& to, const btVector3& color) override;

	virtual int getDebugMode() const override { return debugMode; }

	virtual void reportErrorWarning(const char* warningString) override {}

	virtual void setDebugMode(int debugMode_) override { debugMode = debugMode_; }

	/// Submit all collected lines
	void FlushLines();
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

	/// Draw physical shapes used by bullet3
	void DrawDebug();

	/// Run a simulation with a delta time step
	void StepSimulation(float delta);

	btDiscreteDynamicsWorld* GetWorld() const { return world; }
};
