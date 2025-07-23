// PhysicsWorld.cpp
// @author octopoulos
// @version 2025-07-19

#include "stdafx.h"
#include "PhysicsWorld.h"

#define BGFXH_IMPL
#include <bgfxh/bgfxh.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BulletDebugDraw
//////////////////

void BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	//ui::Log("drawLine");
	if (2 != bgfx::getAvailTransientVertexBuffer(2, bgfxh::PosVertex::ms_decl)) // NOW IT CRASHES HERE, stride = 0
	{
		ui::Log("drawLine: error");
		return;
	}

	bgfx::TransientVertexBuffer vb;
	bgfx::allocTransientVertexBuffer(&vb, 2, bgfxh::PosVertex::ms_decl);
	bgfxh::PosVertex* v = (bgfxh::PosVertex*)vb.data;
	v[0].m_x            = from.x();
	v[0].m_y            = from.y();
	v[0].m_z            = from.z();
	v[1].m_x            = to.x();
	v[1].m_y            = to.y();
	v[1].m_z            = to.z();

	uint8_t  r    = 255 * color.x();
	uint8_t  g    = 255 * color.y();
	uint8_t  b    = 255 * color.z();
	uint8_t  a    = 255;
	uint32_t rgba = (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | (uint32_t(a));

	bgfx::setVertexBuffer(0, &vb);
	//bgfx::setTransform(bgfxModelMtx);

	// BGFX_STATE_MSAA
	bgfx::setState(
		BGFX_STATE_WRITE_RGB | BGFX_STATE_PT_LINES | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_FACTOR, BGFX_STATE_BLEND_ZERO),
		rgba
	);

	// Passthrough - vs emits modelViewProj*pos, fs emits (1,1,1,1)
	bgfx::submit(viewId, bgfxh::m_programUntexturedPassthrough);
	//ui::Log("drawLine: submit");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PhysicsWorld
///////////////

PhysicsWorld::PhysicsWorld()
{
	config     = new btDefaultCollisionConfiguration();
	dispatcher = new btCollisionDispatcher(config);
	broadphase = new btDbvtBroadphase();
	solver     = new btSequentialImpulseConstraintSolver();

	world = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, config);
	world->setGravity(btVector3(0, -9.81f, 0));

	// debug draw
	{
		bgfxh::PosVertex::init();

		debugDraw = std::make_unique<BulletDebugDraw>();
		debugDraw->setDebugMode(0
			//| btIDebugDraw::DBG_DrawAabb
			| btIDebugDraw::DBG_DrawConstraints
			| btIDebugDraw::DBG_DrawWireframe
			);

		world->setDebugDrawer(debugDraw.get());
	}
}

PhysicsWorld::~PhysicsWorld()
{
	if (world && debugDraw)
		world->setDebugDrawer(nullptr);

	delete world;
	delete solver;
	delete broadphase;
	delete dispatcher;
	delete config;
}

void PhysicsWorld::DrawDebug()
{
	//ui::Log("DrawDebug");
	world->debugDrawWorld();
}

/**
 * Run a simulation with a delta time step
 */
void PhysicsWorld::StepSimulation(float delta)
{
	if (world) world->stepSimulation(delta, maxSimulationSteps, 1.0f / 120.0f);
}
