// PhysicsWorld.cpp
// @author octopoulos
// @version 2025-10-14

#include "stdafx.h"
#include "physics/PhysicsWorld.h"
//
#include "materials/ShaderManager.h" // GetShaderManager

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BulletDebugDraw
//////////////////

bgfx::VertexLayout PosColorVertex::ms_layout;

void BulletDebugDraw::drawLine(const btVector3& from, const btVector3& to, const btVector3& color)
{
	// convert color to ABGR
	const uint32_t abgr = (uint32_t(255) << 24) | (uint32_t(255 * color.z()) << 16) | (uint32_t(255 * color.y()) << 8) | uint32_t(255 * color.x());

	// add vertices
	lines.push_back({ from.x(), from.y(), from.z(), abgr });
	lines.push_back({ to.x(), to.y(), to.z(), abgr });

	if (lines.size() >= 8192) FlushLines();
	return;
}

void BulletDebugDraw::FlushLines()
{
	if (lines.empty()) return;

	const uint32_t numVertices = lines.size();
	if (bgfx::getAvailTransientVertexBuffer(numVertices, PosColorVertex::ms_layout) < numVertices)
	{
		ui::Log("FlushLines: Not enough transient buffer space for %d vertices", numVertices);
		lines.clear();
		return;
	}

	bgfx::TransientVertexBuffer vb;
	bgfx::allocTransientVertexBuffer(&vb, numVertices, PosColorVertex::ms_layout);
	memcpy(vb.data, lines.data(), numVertices * sizeof(PosColorVertex));
	bgfx::setVertexBuffer(0, &vb);

	bgfx::setState(0
		| BGFX_STATE_DEPTH_TEST_ALWAYS
		| BGFX_STATE_PT_LINES
		| BGFX_STATE_WRITE_RGB
		| BGFX_STATE_WRITE_Z
	);
	bgfx::submit(viewId, program);

	lines.clear();
}

void BulletDebugDraw::Initialize()
{
	program = GetShaderManager().LoadProgram("vs_cube", "fs_cube");

	setDebugMode(btIDebugDraw::DBG_DrawConstraints | btIDebugDraw::DBG_DrawWireframe); // | btIDebugDraw::DBG_DrawAabb
	lines.reserve(8192);
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
	ui::Log("~PhysicsWorld");
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
		// clang-format off
		PosColorVertex::ms_layout
		    .begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0  , 4, bgfx::AttribType::Uint8, true)
		    .end();
		// clang-format on

		debugDraw = std::make_unique<BulletDebugDraw>();
		debugDraw->Initialize();

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
