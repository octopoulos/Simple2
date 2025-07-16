// Body.cpp
// @author octopoulos
// @version 2025-07-12

#include "stdafx.h"
#include "Body.h"

void Body::CreateBox(float wx, float wy, float wz)
{
	Destroy();

	std::random_device rd;
	std::mt19937       gen(rd());
	auto               distrib = std::uniform_real_distribution<>(0.0, 2.0 * bx::kPi);
	btQuaternion       randRot(distrib(gen), distrib(gen), distrib(gen), 1.0);
	randRot.normalize();

	const auto motion = new btDefaultMotionState(btTransform(randRot, btVector3(0, 5, 0)));
	shape             = new btBoxShape(btVector3(wx, wy, wz));
	const auto ci     = btRigidBody::btRigidBodyConstructionInfo(mass, motion, shape, inertia);

	body = new btRigidBody(ci);
	world->addRigidBody(body);
}

void Body::Destroy()
{
	DestroyBody();
	DestroyShape();
}

void Body::DestroyBody()
{
	if (body)
	{
		if (world) world->removeRigidBody(body);
		delete body->getMotionState();
		delete body;
		body = nullptr;
	}
}

void Body::DestroyShape()
{
	if (shape)
	{
		delete shape;
		shape = nullptr;
	}
}
