// controls.cpp
// @author octopoulos
// @version 2025-07-29

#include "stdafx.h"
#include "app/App.h"
//
#include "common/imgui/imgui.h"
#include "entry/input.h"
#include "loaders/ModelLoader.h"

#define DOWN_OR_REPEAT(key) ((downs[key] || ginput.RepeatingKey(key)) ? 1 : 0)

static inline float SignNonZero(float v)
{
	return (v >= 0.0f) ? 1.0f : -1.0f;
}

void App::FixedControls()
{
	using namespace entry;

	auto& ginput = GetGlobalInput();

	for (int id = 0; id < Key::Count; ++id)
		if (ginput.keyDowns[id]) ui::Log("Controls: {} {:3} {:5} {}", ginput.keyTimes[id], id, ginput.keys[id], getName((Key::Enum)id));

	const auto& downs   = ginput.keyDowns;
	const auto& keys    = ginput.keys;
	const bool  isAlt   = keys[Key::LeftAlt] || keys[Key::RightAlt];
	const bool  isCtrl  = keys[Key::LeftCtrl] || keys[Key::RightCtrl];
	const bool  isShift = keys[Key::LeftShift] || keys[Key::RightShift];

	// 1) alt
	if (isAlt)
	{
	}
	// 2) ctrl
	else if (isCtrl)
	{
	}
	// 3) shift
	else if (isShift)
	{
		if (downs[Key::KeyA])
		{
			ui::Log("Add popup...");
			ImGui::OpenPopup("###popup_Add");
		}
	}
	// 4) no modifier
	else
	{
		// 4.a) new keys
		if (DOWN_OR_REPEAT(Key::KeyO))
		{
			xsettings.physPaused = false;
			pauseNextFrame       = true;
		}
		if (downs[Key::KeyP]) xsettings.physPaused = !xsettings.physPaused;

		if (downs[Key::Key1]) ThrowDonut();

		// move cursor
		{
			static bx::Vec3 cacheForward = bx::InitZero;
			static bx::Vec3 cacheRight   = bx::InitZero;

			// keep used camera position locked until all keys are released
			if (keys[Key::Down] || keys[Key::Left] || keys[Key::Right] || keys[Key::Up])
			{
				// clang-format off
				const int flag = 0
				    | DOWN_OR_REPEAT(Key::Down ) * 1
				    | DOWN_OR_REPEAT(Key::Left ) * 2
				    | DOWN_OR_REPEAT(Key::Right) * 4
				    | DOWN_OR_REPEAT(Key::Up   ) * 8;
				// clang-format on
				if (flag)
				{
					if (flag & (1 | 8))
					{
						const float dir = (flag & 8) ? 1.0f : -1.0f;
						const float fx  = cacheForward.x;
						const float fz  = cacheForward.z;

						if (std::abs(fx) > std::abs(fz))
							cursor->position.x = std::floor(cursor->position.x + dir * SignNonZero(fx)) + 0.5f;
						else
							cursor->position.z = std::floor(cursor->position.z + dir * SignNonZero(fz)) + 0.5f;
					}
					if (flag & (2 | 4))
					{
						const float dir = (flag & 4) ? 1.0f : -1.0f;
						const float fx  = cacheRight.x;
						const float fz  = cacheRight.z;

						if (std::abs(fx) > std::abs(fz))
							cursor->position.x = std::floor(cursor->position.x + dir * SignNonZero(fx)) + 0.5f;
						else
							cursor->position.z = std::floor(cursor->position.z + dir * SignNonZero(fz)) + 0.5f;
					}

					cursor->UpdateLocalMatrix();

					camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(cursor->position));
					camera->Zoom();
				}
			}
			else
			{
				cacheForward = camera->forward;
				cacheRight   = camera->right;
			}
		}

		if (downs[Key::F4]) xsettings.bulletDebug = !xsettings.bulletDebug;
		if (downs[Key::F5]) xsettings.projection = 1 - xsettings.projection;
		if (downs[Key::F11]) xsettings.instancing = !xsettings.instancing;

		if (downs[Key::Tab]) showLearn = !showLearn;

		if (downs[Key::NumPad1]) camera->SetOrthographic({ 0.0f, 0.0f, -1.0f });
		if (downs[Key::NumPad3]) camera->SetOrthographic({ 1.0f, 0.0f, 0.0f });
		if (downs[Key::NumPad7]) camera->SetOrthographic({ 0.0f, 1.0f, -0.1f });
		if (downs[Key::NumPad5]) xsettings.projection = 1 - xsettings.projection;
		// clang-format off
		if (DOWN_OR_REPEAT(Key::NumPad2)) camera->RotateAroundAxis(camera->right  ,  bx::toRad(15.0f));
		if (DOWN_OR_REPEAT(Key::NumPad4)) camera->RotateAroundAxis(camera->worldUp, -bx::toRad(15.0f));
		if (DOWN_OR_REPEAT(Key::NumPad6)) camera->RotateAroundAxis(camera->worldUp,  bx::toRad(15.0f));
		if (DOWN_OR_REPEAT(Key::NumPad8)) camera->RotateAroundAxis(camera->right  , -bx::toRad(15.0f));
		// clang-format on
		if (DOWN_OR_REPEAT(Key::NumPad9))
		{
			for (int i = 0; i < 12; ++i)
			{
				if (i > 0) camera->Update(0.16f);
				camera->RotateAroundAxis(camera->worldUp, -bx::toRad(15.0f));
			}
		}

		// 4.b) holding down
		{
			if (keys[Key::NumPadMinus] || keys[Key::Minus]) camera->Zoom(1.01f);
			if (keys[Key::NumPadPlus] || keys[Key::Equals]) camera->Zoom(1.0f / 1.01f);
		}
	}

	GetGlobalInput().ResetFixed();
	++inputFrame;
}

void App::FluidControls()
{
	auto& ginput = GetGlobalInput();
	ginput.MouseDeltas();

	// camera
	if (!ImGui::MouseOverArea())
	{
		if (ginput.buttons[1] || ginput.mouseLock)
			camera->Orbit(ginput.mouseRels2[0], ginput.mouseRels2[1]);
		camera->Update(deltaTime);
	}

	// holding down
	if (const auto& keys = ginput.keys)
	{
		using namespace entry;

		float speed = deltaTime * 10.0f;
		if (keys[Key::LeftCtrl]) speed *= 0.5f;
		if (keys[Key::LeftShift]) speed *= 2.0f;

		const bool isOrtho = (xsettings.projection == Projection_Orthographic);
		const int  keyQS   = isOrtho ? Key::KeyS : Key::KeyQ;
		const int  keyEW   = isOrtho ? Key::KeyW : Key::KeyE;

		if (keys[Key::KeyA])
		{
			camera->pos2    = bx::mad(camera->right, -speed, camera->pos2);
			camera->target2 = bx::mad(camera->right, -speed, camera->target2);
		}
		if (keys[Key::KeyD])
		{
			camera->pos2    = bx::mad(camera->right, speed, camera->pos2);
			camera->target2 = bx::mad(camera->right, speed, camera->target2);
		}
		if (keys[keyEW])
		{
			camera->pos2    = bx::mad(camera->up, speed, camera->pos2);
			camera->target2 = bx::mad(camera->up, speed, camera->target2);
		}
		if (keys[keyQS])
		{
			camera->pos2    = bx::mad(camera->up, -speed, camera->pos2);
			camera->target2 = bx::mad(camera->up, -speed, camera->target2);
		}
		if (!isOrtho)
		{
			if (keys[Key::KeyS])
			{
				camera->pos2    = bx::mad(camera->forward, -speed, camera->pos2);
				camera->target2 = bx::mad(camera->forward, -speed, camera->target2);
			}
			if (keys[Key::KeyW])
			{
				camera->pos2    = bx::mad(camera->forward, speed, camera->pos2);
				camera->target2 = bx::mad(camera->forward, speed, camera->target2);
			}
		}
	}
}

void App::ThrowDonut()
{
	auto parent = scene->GetObjectByName("donut3-group");

	if (auto object = ModelLoader::LoadModel("donut3"))
	{
		object->type |= ObjectType_Instance;

		const auto  pos    = camera->pos2;
		const float scale  = MerseneFloat(0.25f, 0.75f);
		const float scaleY = scale * MerseneFloat(0.7f, 1.5f);

		object->ScaleRotationPosition(
		    { scale, scaleY, scale },
		    { sinf(0 * 0.3f), 3.0f, 0.0f },
		    { pos.x, pos.y, pos.z }
		);
		object->CreateShapeBody(physics.get(), ShapeType_Cylinder, 1.0f);

		// apply initial FORCE (or impulse) to the body in the camera->forward direction
		const auto impulse = bx::mul(camera->forward, 50.0f);
		for (auto& body : object->bodies)
		{
			// body->body->applyImpulse(BxToBullet(camera->forward), BxToBullet(pos));
			body->body->applyCentralImpulse(BxToBullet(impulse));
		}

		parent->AddNamedChild(object, fmt::format("donut3-{}", parent->children.size()));
	}
}
