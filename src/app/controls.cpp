// controls.cpp
// @author octopoulos
// @version 2025-09-14

#include "stdafx.h"
#include "app/App.h"
//
#include "core/common3d.h"             // PrintMatrix
#include "entry/input.h"               // GetGlobalInput
#include "loaders/MeshLoader.h"        // MeshLoader::
#include "materials/MaterialManager.h" // GetMaterialManager
#include "scenes/Scene.h"              // Scene
//
#include "imgui.h" // ImGui::

enum ThrowActions_ : int
{
	ThrowAction_Drop,
	ThrowAction_Spiral,
	ThrowAction_Throw,
};

static inline float SignNonZero(float v)
{
	return (v >= 0.0f) ? 1.0f : -1.0f;
}

int App::ArrowsFlag()
{
	using namespace entry;

	auto&       ginput  = GetGlobalInput();
	const auto& downs   = ginput.keyDowns;
	const auto& ignores = ginput.keyIgnores;

	// clang-format off
	const int flag = 0
		| GI_REPEAT_CURSOR(Key::Down )     * 1
		| GI_REPEAT_CURSOR(Key::Left )     * 2
		| GI_REPEAT_CURSOR(Key::Right)     * 4
		| GI_REPEAT_CURSOR(Key::Up   )     * 8
		| GI_REPEAT_CURSOR(Key::Quote)     * 16  // y-down
		| GI_REPEAT_CURSOR(Key::Backslash) * 32  // y-up
		| GI_REPEAT_CURSOR(Key::Semicolon) * 32; // y-up
	// clang-format on

	return flag;
}

void App::Controls()
{
	// 1) update time
	{
		curTime   = TO_FLOAT((bx::getHPCounter() - startTime) / TO_DOUBLE(bx::getHPFrequency()));
		deltaTime = curTime - lastTime;
		lastTime  = curTime;
	}

	// 2) controls
	{
		auto& ginput = GetGlobalInput();
		ginput.nowMs = NowMs();
		ginput.PrintKeys();

		MeshControls();
		FluidControls();

		inputLag += deltaTime;
		while (inputLag >= inputDelta)
		{
			FixedControls();
			inputLag -= inputDelta;
		}
	}
}

void App::DeleteSelected()
{
	if (auto target = selectWeak.lock())
	{
		if (!(target->type & (ObjectType_Cursor | ObjectType_Map | ObjectType_Scene)))
		{
			if (auto* parent = target->parent; parent->RemoveChild(target))
			{
				if (parent->type & ObjectType_Map)
				{
					AutoSave();
					SelectObject(nullptr);
				}
			}
		}
	}
}

void App::FixedControls()
{
	using namespace entry;

	auto&          ginput   = GetGlobalInput();
	const auto&    downs    = ginput.keyDowns;
	const auto&    ignores  = ginput.keyIgnores;
	const ImGuiIO& io       = ImGui::GetIO();
	const auto&    keys     = ginput.keys;
	const int      modifier = ginput.IsModifier();

	// ignore inputs when focused on a text input
	if (io.WantTextInput)
	{
	}
	// 2) alt
	else if (modifier & Modifier_Alt)
	{
	}
	// 3) ctrl
	else if (modifier & Modifier_Ctrl)
	{
	}
	// 4) shift
	else if (modifier & Modifier_Shift)
	{
		// unpause + restore physics to every object
		if (GI_DOWN(Key::KeyP))
		{
			xsettings.physPaused = false;
			for (const auto& object : mapNode->children)
			{
				if (auto mesh = Mesh::SharedPtr(object, ObjectType_HasBody))
				{
					ui::Log("enable mesh: {}", mesh->name);
					if (auto& body = mesh->body; !body->enabled)
					{
						mesh->SetBodyTransform();
						mesh->ActivatePhysics(true);
					}
				}
			}
		}

		// print with GUI
		if (GI_DOWN(Key::Print)) wantScreenshot = 1;

		// rotate cursor/selected
		RotateSelected(false);
	}
	// 5) no modifier
	else
	{
		// 5.a) new keys
		if (GI_DOWN(Key::KeyH)) ShowPopup(Popup_AddGeometry);
		if (GI_DOWN(Key::KeyJ)) ShowPopup(Popup_AddMesh);
		if (GI_DOWN(Key::KeyM)) ShowPopup(Popup_AddMap);
		if (GI_DOWN(Key::KeyN)) ShowPopup(Popup_Transform);

		if (GI_REPEAT(Key::KeyO))
		{
			xsettings.physPaused = false;
			pauseNextFrame       = true;
		}
		if (GI_DOWN(Key::KeyP)) xsettings.physPaused = !xsettings.physPaused;
		if (GI_DOWN(Key::KeyT)) showTest = !showTest;
		if (GI_DOWN(Key::KeyX)) ShowPopup(Popup_Delete);

		if (GI_DOWN(Key::Key1)) ThrowMesh(ThrowAction_Throw, "donut3", ShapeType_Cylinder, { "donut_base.png" });
		// TODO: Shape_Capsule doesn't work
		if (GI_DOWN(Key::Key2)) ThrowMesh(ThrowAction_Throw, "kenney_car-kit/taxi", ShapeType_Box);
		if (GI_DOWN(Key::Key3)) ThrowGeometry(ThrowAction_Throw, GeometryType_None, { "colors.png" });
		if (GI_DOWN(Key::Key0))
		{
			if (const auto temp = prevSelWeak.lock())
			{
				prevSelWeak = selectWeak;
				selectWeak  = temp;

				if (auto target = selectWeak.lock())
				{
					camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(target->position));
					camera->Zoom();
				}
			}
		}

		if (GI_DOWN(Key::Return)) SelectObject(nullptr);
		if (GI_DOWN(Key::Esc))
		{
			// object is being placed => cancel
			if (camera->follow & CameraFollow_SelectedObj)
			{
				if (auto target = selectWeak.lock(); target->placing) DeleteSelected();
			}
			hidePopup |= Popup_Any;
		}
		if (GI_DOWN(Key::Tab)) showLearn = !showLearn;

		if (GI_KEY(Key::LeftBracket) || GI_KEY(Key::RightBracket))
		{
			if (auto target = selectWeak.lock(); target && target->parent)
			{
				auto&       parent   = target->parent;
				const auto& children = parent->children;
				const auto  numChild = children.size();
				if (GI_REPEAT(Key::LeftBracket))
				{
					parent->childId = (parent->childId + numChild - 1) % numChild;
					SelectObject(children[parent->childId]);
				}
				if (GI_REPEAT(Key::RightBracket))
				{
					parent->childId = (parent->childId + 1) % numChild;
					SelectObject(children[parent->childId]);
				}
			}
		}

		if (GI_DOWN(Key::F4)) xsettings.bulletDebug = !xsettings.bulletDebug;
		if (GI_DOWN(Key::F5)) xsettings.projection = 1 - xsettings.projection;
		if (GI_DOWN(Key::F11)) xsettings.instancing = !xsettings.instancing;

		// print without GUI
		if (GI_DOWN(Key::Print)) wantScreenshot = 2;

		if (GI_DOWN(Key::Delete)) {}

		// move cursor/selected
		MoveSelected(false);

		if (GI_DOWN(Key::NumPad1)) camera->SetOrthographic({ 0.0f, 0.0f, -1.0f });
		if (GI_DOWN(Key::NumPad3)) camera->SetOrthographic({ 1.0f, 0.0f, 0.0f });
		if (GI_DOWN(Key::NumPad7)) camera->SetOrthographic({ 0.0f, 1.0f, -0.1f });
		if (GI_DOWN(Key::NumPad5)) xsettings.projection = 1 - xsettings.projection;
		// clang-format off
		if (GI_REPEAT(Key::NumPad2)) camera->RotateAroundAxis(camera->right  ,  bx::toRad(15.0f));
		if (GI_REPEAT(Key::NumPad4)) camera->RotateAroundAxis(camera->worldUp, -bx::toRad(15.0f));
		if (GI_REPEAT(Key::NumPad6)) camera->RotateAroundAxis(camera->worldUp,  bx::toRad(15.0f));
		if (GI_REPEAT(Key::NumPad8)) camera->RotateAroundAxis(camera->right  , -bx::toRad(15.0f));
		// clang-format on
		if (GI_REPEAT(Key::NumPad9))
		{
			for (int i = 0; i < 12; ++i)
			{
				if (i > 0) camera->Update(0.16f);
				camera->RotateAroundAxis(camera->worldUp, -bx::toRad(15.0f));
			}
		}

		// 5.b) holding down
		{
			if (GI_KEY(Key::NumPadMinus) || GI_KEY(Key::Minus)) camera->Zoom(1.0f + xsettings.zoomKb * 0.0025f);
			if (GI_KEY(Key::NumPadPlus) || GI_KEY(Key::Equals)) camera->Zoom(1.0f / (1.0f + xsettings.zoomKb * 0.0025f));
		}
	}

	// 6) reset fixed
	GetGlobalInput().ResetFixed();
	++inputFrame;
}

void App::FluidControls()
{
	auto& ginput = GetGlobalInput();
	ginput.MouseDeltas();

	const auto&    ignores  = ginput.keyIgnores;
	const ImGuiIO& io       = ImGui::GetIO();
	const int      modifier = ginput.IsModifier();

	// camera
	{
		if (!io.WantCaptureMouse)
		{
			if ((ginput.buttons[1] || ginput.buttons[2]) || ginput.mouseLock)
				camera->Orbit(ginput.mouseRels2[0], ginput.mouseRels2[1]);

			// typical value: 0.008333325
			if (float wheel = ginput.mouseRels2[2])
			{
				wheel *= xsettings.zoomWheel;
				if (wheel > 0)
					camera->Zoom(1.0f / (1.0f + wheel));
				else
					camera->Zoom(1.0f - wheel);
			}
		}
		camera->Update(deltaTime);
	}

	// ignore inputs when using the GUI?
	if (!io.WantCaptureMouse)
	{
		// mouse clicks
		if (ginput.buttonDowns[1]) PickObject(ginput.mouseAbs[0], ginput.mouseAbs[1]);
		if (ginput.buttonDowns[3]) ShowPopup(Popup_Add);

		// holding key down
		if (const auto& keys = ginput.keys)
		{
			using namespace entry;

			if (GI_KEY(Key::Key4))
			{
				static int64_t prevUs;
				const int64_t  nowUs = NowUs();
				if (nowUs > prevUs + 15 * 1000)
				{
					ThrowMesh(ThrowAction_Spiral, "donut3", ShapeType_Cylinder, { "donut_base.png" });
					prevUs = nowUs;
				}
			}

			float speed = deltaTime * xsettings.cameraSpeed;
			if (modifier & Modifier_Ctrl) speed *= 0.2f;
			if (modifier & Modifier_Shift) speed *= 2.0f;

			const bool isOrtho = (xsettings.projection == Projection_Orthographic);
			const int  keyQS   = isOrtho ? Key::KeyS : Key::KeyQ;
			const int  keyEW   = isOrtho ? Key::KeyW : Key::KeyE;

			if (GI_KEY(Key::KeyA)) camera->Move(CameraDir_Right, -speed);
			if (GI_KEY(Key::KeyD)) camera->Move(CameraDir_Right, speed);
			if (GI_KEY(keyEW)) camera->Move(CameraDir_Up, speed);
			if (GI_KEY(keyQS)) camera->Move(CameraDir_Up, -speed);
			if (!isOrtho)
			{
				if (GI_KEY(Key::KeyS)) camera->Move(CameraDir_Forward, -speed);
				if (GI_KEY(Key::KeyW)) camera->Move(CameraDir_Forward, speed);
			}
		}
	}
}

void App::FocusScreen()
{
	ImGui::SetWindowFocus(nullptr);
}

void App::MeshControls()
{
	if (auto target = selectWeak.lock())
	{
		if (target->type & ObjectType_Mesh)
		{
			auto&       ginput   = GetGlobalInput();
			const auto& downs    = ginput.keyDowns;
			auto&       ignores  = ginput.keyIgnores;
			const auto& keys     = ginput.keys;
			const int   modifier = ginput.IsModifier();

			Mesh::SharedPtr(target)->Controls(camera, modifier, downs, ignores, keys);
		}
	}
}

void App::MoveSelected(bool force)
{
	using namespace entry;

	static bx::Vec3 cacheForward = bx::InitZero;
	static bx::Vec3 cacheRight   = bx::InitZero;

	auto&       ginput  = GetGlobalInput();
	const auto& ignores = ginput.keyIgnores;
	const auto& keys    = ginput.keys;

	// keep used camera position locked until all keys are released
	if (force || GI_KEY(Key::Down) || GI_KEY(Key::Left) || GI_KEY(Key::Right) || GI_KEY(Key::Up) || GI_KEY(Key::Backslash) || GI_KEY(Key::Quote))
	{
		if (const int flag = ArrowsFlag(); flag || force)
		{
			const bool isCursor = camera->follow & CameraFollow_Cursor;
			if (auto target = isCursor ? cursor : selectWeak.lock())
			{
				if (target->posTs > 0.0)
				{
					target->position = target->position2;
					target->posTs    = 0.0;
				}
				if (xsettings.smoothPos)
				{
					target->position1 = target->position;
					target->position2 = target->position;
				}
				auto& result = xsettings.smoothPos ? target->position2 : target->position;

				if (flag & (1 | 8))
				{
					const float dir = (flag & 8) ? 1.0f : -1.0f;
					const float fx  = cacheForward.x;
					const float fz  = cacheForward.z;

					if (bx::abs(fx) > bx::abs(fz))
						result.x = bx::floor(target->position.x * 2.0f + dir * SignNonZero(fx)) * 0.5f;
					else
						result.z = bx::floor(target->position.z * 2.0f + dir * SignNonZero(fz)) * 0.5f;
				}
				if (flag & (2 | 4))
				{
					const float dir = (flag & 4) ? 1.0f : -1.0f;
					const float fx  = cacheRight.x;
					const float fz  = cacheRight.z;

					if (bx::abs(fx) > bx::abs(fz))
						result.x = bx::floor(target->position.x * 2.0f + dir * SignNonZero(fx)) * 0.5f;
					else
						result.z = bx::floor(target->position.z * 2.0f + dir * SignNonZero(fz)) * 0.5f;
				}
				if (flag & (16 | 32))
				{
					const float dir = (flag & 32) ? 1.0f : -1.0f;

					result.y = bx::floor(target->position.y * 2.0f + dir) * 0.5f;
				}

				if (xsettings.smoothPos)
					target->posTs = Nowd();
				else
					target->UpdateLocalMatrix("MoveSelected");

				// move cursor at the same time as the selectWeak?
				if (!isCursor)
				{
					cursor->position    = target->position;
					cursor->position.y  = 1.0f;
					cursor->position1   = target->position1;
					cursor->position1.y = 1.0f;
					cursor->position2   = target->position2;
					cursor->position2.y = 1.0f;
					cursor->posTs       = target->posTs;
					if (!xsettings.smoothPos) cursor->UpdateLocalMatrix("MoveSelected/2");

					// save if moved
					if (flag) AutoSave(target);
				}

				// deactivate physical body
				if (auto mesh = Mesh::SharedPtr(target, ObjectType_HasBody))
				{
					mesh->SetBodyTransform();
					mesh->ActivatePhysics(false);
				}

				const glm::mat4 matrix = target->TransformPosition(result);
				camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(matrix[3]));
				camera->Zoom();
			}
		}
	}
	else
	{
		cacheForward = camera->forward;
		cacheRight   = camera->right;
	}
}

void App::RotateSelected(bool force)
{
	if (const int flag = ArrowsFlag(); flag || force)
	{
		const bool isCursor = camera->follow & CameraFollow_Cursor;
		if (auto target = isCursor ? cursor : selectWeak.lock())
		{
			const int angleInc = xsettings.angleInc;
			if (flag & (1 | 8)) target->irot[0] += (flag & 1) ? -angleInc : angleInc;
			if (flag & (2 | 4)) target->irot[1] += (flag & 2) ? -angleInc : angleInc;
			target->RotationFromIrot(false);
			target->UpdateLocalMatrix("RotateSelected");

			// rotate cursor as the same time as the selectWeak?
			if (!isCursor)
			{
				memcpy(cursor->irot, target->irot, sizeof(cursor->irot));

				cursor->quaternion  = target->quaternion;
				cursor->quaternion1 = target->quaternion;
				cursor->quaternion2 = target->quaternion;
				cursor->quatTs      = target->quatTs;
				if (!xsettings.smoothPos) cursor->UpdateLocalMatrix("RotateSelected/2");

				// save if rotated
				if (flag) AutoSave(target);
			}

			// deactivate physical body
			if (auto mesh = Mesh::SharedPtr(target, ObjectType_HasBody))
			{
				mesh->SetBodyTransform();
				mesh->ActivatePhysics(false);
			}
		}
	}
}

void App::ThrowGeometry(int action, int geometryType, const VEC_STR& texFiles)
{
	if (geometryType == GeometryType_None)
		geometryType = MerseneInt32(GeometryType_Box, GeometryType_Count - 1);

	// 1) get/create the parent
	const auto name      = fmt::format("geom-{}", geometryType);
	auto       groupName = fmt::format("{}-group", name);

	Mesh* parent = nullptr;
	if (auto parentObj = Scene::SharedPtr(scene)->GetObjectByName(groupName))
	{
		if (parentObj->type & ObjectType_Mesh)
			parent = static_cast<Mesh*>(parentObj.get());
	}
	else if (auto mesh = std::make_shared<Mesh>(std::move(groupName), ObjectType_Group | ObjectType_Instance))
	{
		mesh->geometry = CreateAnyGeometry(geometryType);
		mesh->material = GetMaterialManager().LoadMaterial(fmt::format("model-inst:{}", ArrayJoin(texFiles, '-')), "vs_model_texture_instance", "fs_model_texture_instance", texFiles);
		scene->AddChild(mesh);

		parent = mesh.get();
		ui::Log("ThrowGeometry: new parent: {}", name);
	}
	if (!parent) return;

	// 2) clone an instance
	if (auto object = parent->CloneInstance(fmt::format("{}:{}", name, parent->children.size())))
	{
		object->material = GetMaterialManager().LoadMaterial(fmt::format("model:{}", ArrayJoin(texFiles, '-')), "vs_model_texture", "fs_model_texture", texFiles);

		const auto  pos   = camera->pos2;
		const float scale = bx::clamp(NormalFloat(1.0f, 0.2f), 0.25f, 1.5f);

		object->ScaleIrotPosition(
		    { scale, scale, scale },
		    { 0, 0, 0 },
		    { pos.x, pos.y, pos.z });
		object->CreateShapeBody(GetPhysics(), GeometryShape(geometryType), 1.0f);

		if (action == ThrowAction_Throw)
		{
			const auto impulse = bx::mul(camera->forward, 40.0f);
			const auto offset  = btVector3(NormalFloat(), NormalFloat(), NormalFloat()) * 0.02f * scale;
			object->body->body->applyImpulse(BxToBullet(impulse), offset);
		}

		parent->AddChild(std::move(object));
	}
}

void App::ThrowMesh(int action, std::string_view name, int shapeType, const VEC_STR& texFiles)
{
	// 1) get/create the parent
	auto groupName = fmt::format("{}-group", name);

	Mesh* parent = nullptr;
	if (auto parentObj = Scene::SharedPtr(scene)->GetObjectByName(groupName))
	{
		if (parentObj->type & ObjectType_Mesh)
			parent = static_cast<Mesh*>(parentObj.get());
	}
	else if (auto mesh = MeshLoader::LoadModelFull(groupName, name, texFiles))
	{
		mesh->type |= ObjectType_Group | ObjectType_Instance;
		mesh->material->LoadProgram("vs_model_texture_instance", "fs_model_texture_instance");
		scene->AddChild(mesh);

		parent = mesh.get();
		ui::Log("ThrowMesh: new parent: {}", name);
	}
	if (!parent) return;

	// 2) clone an instance
	if (auto object = parent->CloneInstance(fmt::format("{}:{}", name, parent->children.size())))
	{
		object->material = GetMaterialManager().LoadMaterial(fmt::format("model:{}", ArrayJoin(texFiles, '-')), "vs_model_texture", "fs_model_texture", texFiles);

		bx::Vec3 pos = camera->pos2;
		bx::Vec3 rot = bx::InitZero;

		if (action == ThrowAction_Spiral)
		{
			const auto numChild = parent->children.size();
			if (numChild)
			{
				const auto& child = parent->children.back();
				const auto& cpos  = child->matrixWorld[3];
				ui::Log(" - {}: {} {} {}", child->name, cpos[0], cpos[1], cpos[2]);
				pos.y = bx::max(pos.x, cpos[1] + 0.5f);
			}
			else pos.y = 10.0f;

			const float radius = sinf(numChild * 0.02f) * 7.0f;
			{
				pos.x = cosf(numChild * 0.2f) * radius;
				pos.z = sinf(numChild * 0.2f) * radius;
				rot.x = sinf(numChild * 0.3f);
				rot.y = 3.0f;
			}
			if (numChild & 1) shapeType = ShapeType_Box;
		}

		const float scale  = MerseneFloat(0.25f, 1.0f);
		const float scaleY = scale * MerseneFloat(0.7f, 1.5f);

		object->ScaleRotationPosition(
		    { scale, scaleY, scale },
		    { rot.x, rot.y, rot.z },
		    { pos.x, pos.y, pos.z }
		);
		object->CreateShapeBody(GetPhysics(), shapeType, 1.0f);

		if (action == ThrowAction_Throw)
		{
			const auto impulse = bx::mul(camera->forward, 40.0f);
			const auto offset  = btVector3(NormalFloat(), NormalFloat(), NormalFloat()) * 0.02f * scale;
			object->body->body->applyImpulse(BxToBullet(impulse), offset);
		}

		parent->AddChild(object);
	}
}
