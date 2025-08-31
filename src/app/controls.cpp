// controls.cpp
// @author octopoulos
// @version 2025-08-27

#include "stdafx.h"
#include "app/App.h"
//
#include "common/config.h"
#include "common/imgui/imgui.h"
#include "core/common3d.h"
#include "entry/input.h"
#include "loaders/MeshLoader.h"
#include "materials/MaterialManager.h"

enum ThrowActions_ : int
{
	ThrowAction_Drop,
	ThrowAction_Spiral,
	ThrowAction_Throw,
};

#define DOWN_OR_REPEAT(key) ((downs[key] || ginput.RepeatingKey(key)) ? 1 : 0)

static inline float SignNonZero(float v)
{
	return (v >= 0.0f) ? 1.0f : -1.0f;
}

int App::ArrowsFlag()
{
	using namespace entry;

	auto&       ginput = GetGlobalInput();
	const auto& downs  = ginput.keyDowns;

	// clang-format off
	const int flag = 0
		| DOWN_OR_REPEAT(Key::Down )     * 1
		| DOWN_OR_REPEAT(Key::Left )     * 2
		| DOWN_OR_REPEAT(Key::Right)     * 4
		| DOWN_OR_REPEAT(Key::Up   )     * 8
		| DOWN_OR_REPEAT(Key::Quote)     * 16  // y-down
		| DOWN_OR_REPEAT(Key::Backslash) * 32; // y-up
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
		GetGlobalInput().nowMs = NowMs();

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

	auto& ginput = GetGlobalInput();

	if (DEV_controls)
	{
		for (int id = 0; id < Key::Count; ++id)
			if (ginput.keyDowns[id]) ui::Log("Controls: {} {:3} {:5} {}", ginput.keyTimes[id], id, ginput.keys[id], getName((Key::Enum)id));
	}

	const auto&    downs    = ginput.keyDowns;
	const ImGuiIO& io       = ImGui::GetIO();
	const auto&    keys     = ginput.keys;
	const int      modifier = ginput.IsModifier();

	// ignore inputs when focused on a text input
	if (io.WantTextInput)
	{
	}
	// 1) alt
	else if (modifier & Modifier_Alt)
	{
	}
	// 2) ctrl
	else if (modifier & Modifier_Ctrl)
	{
	}
	// 3) shift
	else if (modifier & Modifier_Shift)
	{
		// unpause + restore physics to every object
		if (downs[Key::KeyP])
		{
			xsettings.physPaused = false;
			for (const auto& object : scene->children)
			{
				if (auto mesh = Mesh::SharedPtr(object, ObjectType_HasBody))
				{
					if (auto& body = mesh->body; !body->enabled)
					{
						mesh->SetBodyTransform();
						mesh->ActivatePhysics(true);
					}
				}
			}
		}

		// print with GUI
		if (downs[Key::Print]) wantScreenshot = 1;

		// rotate object
		if (const int flag = ArrowsFlag())
		{
			if (auto target = (camera->follow & CameraFollow_Cursor) ? cursor : selectWeak.lock())
			{
				const int angleInc = xsettings.angleInc;
				if (flag & (1 | 8)) target->irot[0] += (flag & 1) ? -angleInc : angleInc;
				if (flag & (2 | 4)) target->irot[1] += (flag & 2) ? -angleInc : angleInc;
				target->RotationFromIrot(false);
				target->UpdateLocalMatrix("FixedControls");

				// deactivate physical body
				if (auto mesh = Mesh::SharedPtr(target, ObjectType_HasBody))
				{
					mesh->SetBodyTransform();
					mesh->ActivatePhysics(false);
				}

				// save?
				AutoSave(target);
			}
		}
	}
	// 4) no modifier
	else
	{
		// 4.a) new keys
		if (downs[Key::KeyH]) ShowPopup(Popup_AddGeometry);
		if (downs[Key::KeyJ]) ShowPopup(Popup_AddMesh);
		if (downs[Key::KeyM]) ShowPopup(Popup_AddMap);
		if (downs[Key::KeyN]) ShowPopup(Popup_Transform);

		if (DOWN_OR_REPEAT(Key::KeyO))
		{
			xsettings.physPaused = false;
			pauseNextFrame       = true;
		}
		if (downs[Key::KeyP]) xsettings.physPaused = !xsettings.physPaused;
		if (downs[Key::KeyX]) ShowPopup(Popup_Delete);

		if (downs[Key::Key1]) ThrowMesh(ThrowAction_Throw, "donut3", ShapeType_Cylinder, "donut_base.png");
		// TODO: Shape_Capsule doesn't work
		if (downs[Key::Key2]) ThrowMesh(ThrowAction_Throw, "kenney_car-kit/taxi", ShapeType_Box);
		if (downs[Key::Key3]) ThrowGeometry(ThrowAction_Throw, GeometryType_None, "colors.png");
		if (downs[Key::Key0])
		{
			if (const auto& temp = prevSelWeak.lock())
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

		if (downs[Key::Return]) SelectObject(nullptr);
		if (downs[Key::Esc])
		{
			// object is being placed => cancel
			if (camera->follow & CameraFollow_SelectedObj)
			{
				if (auto target = selectWeak.lock(); !target->placed) DeleteSelected();
			}
			hidePopup |= Popup_Any;
		}
		if (downs[Key::Tab]) showLearn = !showLearn;

		if (const auto numChild = mapNode->children.size())
		{
			if (DOWN_OR_REPEAT(Key::LeftBracket))
			{
				mapNode->childId = (mapNode->childId + numChild - 1) % numChild;
				SelectObject(mapNode->children[mapNode->childId]);
			}
			if (DOWN_OR_REPEAT(Key::RightBracket))
			{
				mapNode->childId = (mapNode->childId + 1) % numChild;
				SelectObject(mapNode->children[mapNode->childId]);
			}
		}

		if (downs[Key::F4]) xsettings.bulletDebug = !xsettings.bulletDebug;
		if (downs[Key::F5]) xsettings.projection = 1 - xsettings.projection;
		if (downs[Key::F11]) xsettings.instancing = !xsettings.instancing;

		// print without GUI
		if (downs[Key::Print]) wantScreenshot = 2;

		if (downs[Key::Delete]) {}

		// move cursor
		MoveCursor(false);

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
			if (keys[Key::NumPadMinus] || keys[Key::Minus]) camera->Zoom(1.0f + xsettings.zoomKb * 0.0025f);
			if (keys[Key::NumPadPlus] || keys[Key::Equals]) camera->Zoom(1.0f / (1.0f + xsettings.zoomKb * 0.0025f));
		}
	}

	GetGlobalInput().ResetFixed();
	++inputFrame;
}

void App::FluidControls()
{
	auto& ginput = GetGlobalInput();
	ginput.MouseDeltas();

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
		if (ginput.buttonDowns[3]) ShowPopup(Popup_Add);

		// holding key down
		if (const auto& keys = ginput.keys)
		{
			using namespace entry;

			if (keys[Key::Key4])
			{
				static int64_t prevUs;
				const int64_t  nowUs = NowUs();
				if (nowUs > prevUs + 15 * 1000)
				{
					ThrowMesh(ThrowAction_Spiral, "donut3", ShapeType_Cylinder, "donut_base.png");
					prevUs = nowUs;
				}
			}

			float speed = deltaTime * xsettings.cameraSpeed;
			if (modifier & Modifier_Ctrl) speed *= 0.2f;
			if (modifier & Modifier_Shift) speed *= 2.0f;

			const bool isOrtho = (xsettings.projection == Projection_Orthographic);
			const int  keyQS   = isOrtho ? Key::KeyS : Key::KeyQ;
			const int  keyEW   = isOrtho ? Key::KeyW : Key::KeyE;

			if (keys[Key::KeyA]) camera->Move(CameraDir_Right, -speed);
			if (keys[Key::KeyD]) camera->Move(CameraDir_Right, speed);
			if (keys[keyEW]) camera->Move(CameraDir_Up, speed);
			if (keys[keyQS]) camera->Move(CameraDir_Up, -speed);
			if (!isOrtho)
			{
				if (keys[Key::KeyS]) camera->Move(CameraDir_Forward, -speed);
				if (keys[Key::KeyW]) camera->Move(CameraDir_Forward, speed);
			}
		}
	}
}

void App::FocusScreen()
{
	ImGui::SetWindowFocus(nullptr);
}

void App::MoveCursor(bool force)
{
	using namespace entry;

	static bx::Vec3 cacheForward = bx::InitZero;
	static bx::Vec3 cacheRight   = bx::InitZero;

	auto&       ginput = GetGlobalInput();
	const auto& keys   = ginput.keys;

	// keep used camera position locked until all keys are released
	if (force || keys[Key::Down] || keys[Key::Left] || keys[Key::Right] || keys[Key::Up] || keys[Key::Backslash] || keys[Key::Quote])
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
					target->UpdateLocalMatrix("MoveCursor");

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
					if (!xsettings.smoothPos) cursor->UpdateLocalMatrix("MoveCursor2");

					// save?
					AutoSave(target);
				}

				// deactivate physical body
				if (auto mesh = Mesh::SharedPtr(target, ObjectType_HasBody))
				{
					mesh->SetBodyTransform();
					mesh->ActivatePhysics(false);
				}

				camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(result));
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

void App::SelectObject(const sObject3d& obj, bool countIndex)
{
	// 1) place + restore material
	if (const auto& temp = selectWeak.lock())
	{
		temp->placed = true;

		if (auto mesh = Mesh::SharedPtr(temp); mesh->material0)
			mesh->material = mesh->material0;
	}

	// 2) new selection
	if (!obj)
	{
		camera->follow  = CameraFollow_Cursor;
		cursor->visible = true;
	}
	else
	{
		prevSelWeak = selectWeak;
		selectWeak  = obj;

		camera->follow |= CameraFollow_Active | CameraFollow_SelectedObj;
		camera->follow &= ~CameraFollow_Cursor;

		if (auto mesh = Mesh::SharedPtr(obj))
		{
			mesh->material0 = mesh->material;
			mesh->material  = GetMaterialManager().GetMaterial("cursor");
			cursor->visible = false;
		}

		// find the parent's childId
		if (countIndex)
		{
			if (auto* parent = obj->parent)
			{
				for (int id = -1; const auto& child : parent->children)
				{
					++id;
					if (child == obj)
					{
						parent->childId = id;
						break;
					}
				}
			}
		}
	}

	// 3) camera on target
	if (const sObject3d target = obj ? obj : cursor)
	{
		camera->target2 = bx::load<bx::Vec3>(glm::value_ptr(target->position));
		camera->Zoom();

		MoveCursor(true);
		if (DEV_matrix) PrintMatrix(target->matrixWorld, target->name);
		FocusScreen();
	}
}

void App::ThrowGeometry(int action, int geometryType, std::string_view textureName)
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
		mesh->material = GetMaterialManager().LoadMaterial(fmt::format("model-inst:{}", textureName), "vs_model_texture_instance", "fs_model_texture_instance", textureName);
		scene->AddChild(mesh);

		parent = mesh.get();
		ui::Log("ThrowGeometry: new parent: {}", name);
	}
	if (!parent) return;

	// 2) clone an instance
	if (auto object = parent->CloneInstance(fmt::format("{}:{}", name, parent->children.size())))
	{
		object->material = GetMaterialManager().LoadMaterial(fmt::format("model:{}", textureName), "vs_model_texture", "fs_model_texture", textureName);

		const auto  pos   = camera->pos2;
		const float scale = std::clamp(NormalFloat(1.0f, 0.2f), 0.25f, 1.5f);

		object->ScaleIrotPosition(
		    { scale, scale, scale },
		    { 0, 0, 0 },
		    { pos.x, pos.y, pos.z });
		object->CreateShapeBody(physics.get(), GeometryShape(geometryType), 1.0f);

		if (action == ThrowAction_Throw)
		{
			const auto impulse = bx::mul(camera->forward, 40.0f);
			const auto offset  = btVector3(NormalFloat(), NormalFloat(), NormalFloat()) * 0.02f * scale;
			object->body->body->applyImpulse(BxToBullet(impulse), offset);
		}

		parent->AddChild(std::move(object));
	}
}

void App::ThrowMesh(int action, std::string_view name, int shapeType, std::string_view textureName)
{
	// 1) get/create the parent
	auto groupName = fmt::format("{}-group", name);

	Mesh* parent = nullptr;
	if (auto parentObj = Scene::SharedPtr(scene)->GetObjectByName(groupName))
	{
		if (parentObj->type & ObjectType_Mesh)
			parent = static_cast<Mesh*>(parentObj.get());
	}
	else if (auto mesh = MeshLoader::LoadModelFull(groupName, name, textureName))
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
		object->material = GetMaterialManager().LoadMaterial(fmt::format("model:{}", textureName), "vs_model_texture", "fs_model_texture", textureName);

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
				pos.y = std::max(pos.x, cpos[1] + 0.5f);
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
		object->CreateShapeBody(physics.get(), shapeType, 1.0f);

		if (action == ThrowAction_Throw)
		{
			const auto impulse = bx::mul(camera->forward, 40.0f);
			const auto offset  = btVector3(NormalFloat(), NormalFloat(), NormalFloat()) * 0.02f * scale;
			object->body->body->applyImpulse(BxToBullet(impulse), offset);
		}

		parent->AddChild(object);
	}
}
