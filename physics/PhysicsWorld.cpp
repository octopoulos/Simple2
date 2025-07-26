// PhysicsWorld.cpp
// @author octopoulos
// @version 2025-07-22

#include "stdafx.h"
#include "PhysicsWorld.h"

#define BGFXH_IMPL
#include <bgfxh/bgfxh.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BulletDebugDraw
//////////////////

bgfx::VertexLayout PosColorVertex::ms_layout;

void BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	// new method
	if (1)
	{
		// Convert color to ABGR
		const uint8_t  r    = 255 * color.x();
		const uint8_t  g    = 255 * color.y();
		const uint8_t  b    = 255 * color.z();
		const uint8_t  a    = 255;
		const uint32_t abgr = (uint32_t(a) << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
		uint32_t       rgba = (uint32_t(r) << 24) | (uint32_t(g) << 16) | (uint32_t(b) << 8) | (uint32_t(a));

		// Add vertices
		lines.push_back({ from.x(), from.y(), from.z(), rgba });
		lines.push_back({ to.x(), to.y(), to.z(), rgba });

		if (lines.size() >= 8192) FlushLines();
		return;
	}

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
}

void BulletDebugDraw::FlushLines()
{
	if (lines.empty()) return;

	const uint32_t numVertices = lines.size();
	if (bgfx::getAvailTransientVertexBuffer(numVertices, PosColorVertex::ms_layout) < numVertices)
	{
		ui::Log("flushLines: Not enough transient buffer space for {} vertices", numVertices);
		lines.clear();
		return;
	}

	bgfx::TransientVertexBuffer vb;
	bgfx::allocTransientVertexBuffer(&vb, numVertices, PosColorVertex::ms_layout);
	std::memcpy(vb.data, lines.data(), numVertices * sizeof(PosColorVertex));

	bgfx::setVertexBuffer(0, &vb);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_PT_LINES | BGFX_STATE_BLEND_ALPHA);

	// Passthrough shader
	bgfx::submit(viewId, bgfxh::m_programUntexturedPassthrough);

	lines.clear();
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
	// initialize debugDraw once
	if (!debugDraw)
	{
		bgfxh::PosVertex::init();

		// clang-format off
		PosColorVertex::ms_layout
		    .begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0  , 4, bgfx::AttribType::Uint8, true)
		    .end();
		// clang-format on

		debugDraw = std::make_unique<BulletDebugDraw>();
		debugDraw->setDebugMode(btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawWireframe); // | btIDebugDraw::DBG_DrawAabb
		debugDraw->lines.reserve(8192);

		world->setDebugDrawer(debugDraw.get());
	}

	debugDraw->viewId = 0;

	world->debugDrawWorld();
	debugDraw->FlushLines();
}

void PhysicsWorld::StepSimulation(float delta)
{
	//if (world) world->stepSimulation(delta, maxSimulationSteps, 1.0f / 120.0f);
	if (world) world->stepSimulation(delta, 5, 1.0f / 120.0f);
}
