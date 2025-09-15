// RubikCube.cpp
// @author octopoulos
// @version 2025-09-11

#include "stdafx.h"
#include "objects/RubikCube.h"
//
#include "common/config.h"             // DEV_rubik
#include "core/common3d.h"             // BxToGlm
#include "entry/input.h"               // GetGlobalInput
#include "geometries/Geometry.h"       // uGeometry
#include "materials/MaterialManager.h" // GetMaterialManager
#include "loaders/writer.h"            // WRITE_KEY_xxx
#include "ui/ui.h"                     // ui::
#include "ui/xsettings.h"              // xsettings

static const std::vector<RubikFace> rubikStartFaces = {
	{ "", "yellow", {  0.0f,  1.0f,  0.0f } },
	{ "", "white" , {  0.0f, -1.0f,  0.0f } },
	{ "", "red"   , {  1.0f,  0.0f,  0.0f } },
	{ "", "orange", { -1.0f,  0.0f,  0.0f } },
	{ "", "green" , {  0.0f,  0.0f,  1.0f } },
	{ "", "blue"  , {  0.0f,  0.0f, -1.0f } },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RubikCube::AiControls(const sCamera& camera, int modifier, const bool* downs)
{
	using namespace entry;

	static bool ignores[256] = {};

	// 1) detect faces
	// - rotate the normals
	auto faces = rubikStartFaces;
	for (auto& face : faces)
		face.normalWorld = quaternion * face.normal;

	const auto backward = -BxToGlm(camera->forward);
	const auto right    = BxToGlm(camera->right);
	const auto worldUp  = BxToGlm(camera->worldUp);

	float            frontMaxDot  = -2.0f;
	const RubikFace* frontMaxFace = nullptr;
	float            frontMinDot  = 2.0f;
	const RubikFace* frontMinFace = nullptr;
	float            rightMaxDot  = -2.0f;
	const RubikFace* rightMaxFace = nullptr;
	float            rightMinDot  = 2.0f;
	const RubikFace* rightMinFace = nullptr;
	float            upMaxDot     = -2.0f;
	const RubikFace* upMaxFace    = nullptr;
	float            upMinDot     = 2.0f;
	const RubikFace* upMinFace    = nullptr;

	// 1.a) find up/down
	for (const auto& face : faces)
	{
		const float dot = glm::dot(face.normalWorld, worldUp);
		if (dot > upMaxDot)
		{
			upMaxDot  = dot;
			upMaxFace = &face;
		}
		if (dot < upMinDot)
		{
			upMinDot  = dot;
			upMinFace = &face;
		}
	}
	if (!upMaxFace || !upMinFace)
	{
		ui::LogError("RubikCube/Controls: no up/down");
		return;
	}

	// 1.b) find other faces
	for (const auto& face : faces)
	{
		if (&face == upMaxFace || &face == upMinFace) continue;

		// front
		{
			const float dot = glm::dot(face.normalWorld, backward);
			if (dot > frontMaxDot)
			{
				frontMaxDot  = dot;
				frontMaxFace = &face;
			}
			if (dot < frontMinDot)
			{
				frontMinDot  = dot;
				frontMinFace = &face;
			}
		}
		// right
		{
			const float dot = glm::dot(face.normalWorld, right);
			if (dot > rightMaxDot)
			{
				rightMaxDot  = dot;
				rightMaxFace = &face;
			}
			if (dot < rightMinDot)
			{
				rightMinDot  = dot;
				rightMinFace = &face;
			}
		}
	}
	if (!frontMaxFace || !frontMinFace || !rightMaxFace || !rightMinFace)
	{
		ui::LogError("RubikCube/Controls: no front/back/right/left");
		return;
	}

	const float frontVis = frontMaxDot;
	const float rightVis = glm::dot(rightMaxFace->normalWorld, backward);
	const float upVis    = glm::dot(upMaxFace->normalWorld   , backward);

	if (DEV_rubik)
	{
		ui::Log("FRONT: maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", frontMaxDot, frontMaxFace->name, frontVis, frontMinDot, frontMinFace->name, glm::dot(frontMinFace->normalWorld, backward));
		ui::Log("RIGHT: maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", rightMaxDot, rightMaxFace->name, rightVis, rightMinDot, rightMinFace->name, glm::dot(rightMinFace->normalWorld, backward));
		ui::Log("UP   : maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", upMaxDot   , upMaxFace->name   , upVis   , upMinDot   , upMinFace->name   , glm::dot(upMinFace->normalWorld   , backward));
	}

	// 2) whole cube rotation
	int angle = 90;
	if (modifier & Modifier_Shift) angle *= -1;
	if (modifier & Modifier_Meta) angle *= 2;

	// clang-format off
	if (GI_DOWN(Key::KeyX)) RotateCube(rightMaxFace, angle); // x-axis
	if (GI_DOWN(Key::KeyY)) RotateCube(upMaxFace   , angle); // y-axis
	if (GI_DOWN(Key::KeyZ)) RotateCube(frontMaxFace, angle); // z-axis
	// clang-format off

	// 3) handle layer rotations
	// clang-format off
	if (GI_DOWN(Key::KeyB)) RotateLayer(frontMinFace, angle);
	if (GI_DOWN(Key::KeyD)) RotateLayer(upMinFace   , angle);
	if (GI_DOWN(Key::KeyF)) RotateLayer(frontMaxFace, angle);
	if (GI_DOWN(Key::KeyL)) RotateLayer(rightMinFace, angle);
	if (GI_DOWN(Key::KeyR)) RotateLayer(rightMaxFace, angle);
	if (GI_DOWN(Key::KeyU)) RotateLayer(upMaxFace   , angle);
	// clang-format on

	// 4) apply changes
	if (isDirty)
	{
		RotationFromQuaternion();
		UpdateLocalMatrix("Controls");
		isDirty = false;
	}
}

/// https://www.gancube.com/pages/3x3x3-cfop-guide-of-gancube
void RubikCube::Controls(const sCamera& camera, int modifier, const bool* downs, bool* ignores, const bool* keys)
{
	using namespace entry;

	static const USET_INT newIgnores = {
		Key::KeyB,
		Key::KeyD,
		// Key::KeyE,
		Key::KeyF,
		Key::KeyK,
		Key::KeyL,
		// Key::KeyM,
		Key::KeyR,
		// Key::KeyS,
		Key::KeyU,
		Key::KeyX,
		Key::KeyY,
		Key::KeyZ,
	};

	// 1) scramble?
	auto& ginput = GetGlobalInput();
	if (GI_REPEAT_RUBIK(Key::KeyK))
		Scramble(camera, 1);
	// 2) AI controls
	else
		AiControls(camera, modifier, downs);

	// 3) set ignored keys
	for (const int ignore : newIgnores)
		ignores[ignore] = true;
}

void RubikCube::Initialize()
{
	load = MeshLoad_Full;

	// create material for all cubies
	const auto cubieMaterial = GetMaterialManager().LoadMaterial("rubik_cubie", "vs_rubik", "fs_rubik");

	// define face colors (RGBA)
	const std::vector<glm::vec4> faceColors = {
		glm::vec4(0.95f, 0.00f, 0.00f, 1.00f), // +x (red)
		glm::vec4(1.10f, 0.70f, 0.00f, 1.00f), // -x (orange)
		glm::vec4(1.00f, 0.95f, 0.00f, 1.00f), // +y (yellow)
		glm::vec4(1.10f, 1.10f, 1.10f, 1.00f), // -y (white)
		glm::vec4(0.00f, 0.85f, 0.00f, 1.00f), // +z (green)
		glm::vec4(0.00f, 0.60f, 0.95f, 1.00f), // -z (blue)
	};
	const glm::vec4 defaultColor(0.85f, 0.85f, 0.85f, 1.0f); // black/gray for inner faces

	// total size of each cubie (edge length)
	const float cubeEdge = 0.95f;               // slightly larger than 1.0 for visible gaps
	const float spacing  = 1.0f;                // distance between cubie centers
	const int   S        = cubeSize - 1;        // shortcut
	const float offset   = -S * spacing / 2.0f; // center the cube at origin

	// create n^3 cubies
	for (int x = 0; x < cubeSize; ++x)
	{
		for (int y = 0; y < cubeSize; ++y)
		{
			for (int z = 0; z < cubeSize; ++z)
			{
				// name each cubie for identification
				const std::string cubieName = fmt::format("Cubie_{}_{}_{}", x, y, z);

				// determine face colors based on position
				std::vector<glm::vec4> cubieFaceColors(6, defaultColor);
				if (x == S) cubieFaceColors[0] = faceColors[0]; // +x
				if (x == 0) cubieFaceColors[1] = faceColors[1]; // -x
				if (y == S) cubieFaceColors[2] = faceColors[2]; // +y
				if (y == 0) cubieFaceColors[3] = faceColors[3]; // -y
				if (z == S) cubieFaceColors[4] = faceColors[4]; // +z
				if (z == 0) cubieFaceColors[5] = faceColors[5]; // -z

				// use shared geometry for inner cubies
				const bool isInnerCubie  = (x > 0 && x < S && y > 0 && y < S && z > 0 && z < S);
				uGeometry  cubieGeometry = CreateBoxGeometry(cubeEdge, cubeEdge, cubeEdge, 1, 1, 1, cubieFaceColors);

				// create a mesh for the cubie
				const auto cubie = std::make_shared<Mesh>(cubieName, ObjectType_RubikNode, cubieGeometry, cubieMaterial);

				// position the cubie in the 3D grid
				const glm::vec3 position(
				    offset + x * spacing,
				    offset + y * spacing,
				    offset + z * spacing);

				const glm::vec3 scale(1.0f);
				cubie->ScaleQuaternionPosition(scale, glm::identity<glm::quat>(), position);

				// add cubie as a child
				AddChild(cubie);
			}
		}
	}

	ui::Log("RubikCube: Created {} cubies for {}x{}x{} cube", cubeSize * cubeSize * cubeSize, cubeSize, cubeSize, cubeSize);

	// update world matrices for all children
	UpdateWorldMatrix(true);
}

void RubikCube::RotateCube(const RubikFace* face, int angle)
{
	// 1) handle non completed interpolation
	const int change = CompleteInterpolation();
	ui::Log("RotateCube: {}", change);

	// 2) new interpolation
	{
		const glm::vec3& axis    = face->normalWorld;
		const glm::quat  rotQuat = glm::angleAxis(bx::toRad(angle), axis);

		quaternion1 = quaternion;
		quaternion2 = glm::normalize(rotQuat * quaternion);
		quatTs      = Nowd();
		isDirty     = true;
	}
}

void RubikCube::RotateLayer(const RubikFace* face, int angle)
{
	// 1) stop current interpolation
	const int change = CompleteInterpolation();
	ui::Log("RotateLayer: {}", change);

	// 2) find layer cubies
	std::vector<sObject3d> cubies     = {};
	const glm::vec3&       faceNormal = face->normalWorld;
	const glm::vec3        rubikPos   = matrixWorld[3];
	const float            threshold  = 0.5f;

	for (const auto& child : children)
	{
		const glm::vec3 worldPos = child->matrixWorld[3];
		const float     proj     = glm::dot(worldPos - rubikPos, faceNormal);
		if (proj >= threshold) cubies.push_back(child);
	}

	// 3) rotate if the number of cubies match the expected result
	if (cubies.size() == cubeSize * cubeSize)
	{
		const float     angleInc = bx::toRad(angle);
		const glm::quat rotQuat  = glm::angleAxis(angleInc, face->normal);

		for (auto& cubie : cubies)
		{
			// rotate local position around local axis (0,0,0) + snap to grid
			{
				glm::vec3 newLocalPos = rotQuat * cubie->position;
				newLocalPos = glm::floor(newLocalPos * 2.0f + 0.5f) * 0.5f;

				cubie->axis1       = glm::identity<glm::quat>();
				cubie->axis2       = rotQuat;
				cubie->position1   = cubie->position;
				cubie->position2   = newLocalPos;
				cubie->quaternion1 = cubie->quaternion;
				cubie->quaternion2 = glm::normalize(rotQuat * cubie->quaternion);

				cubie->axisTs = Nowd();
				cubie->quatTs = Nowd();
			}
		}
		isDirty = true;
	}
	else ui::Log("\ncubies={}\n", cubies.size());
}

void RubikCube::Scramble(const sCamera& camera, int steps)
{
	using namespace entry;

	for (int step = 0; step < steps; ++step)
	{
		static const VEC_INT keys = {
			Key::KeyB,
			Key::KeyD,
			// Key::KeyE,
			Key::KeyF,
			Key::KeyL,
			// Key::KeyM,
			Key::KeyR,
			// Key::KeyS,
			Key::KeyU,
			Key::KeyX,
			Key::KeyY,
			Key::KeyZ,
		};

		bool downs[256] = {};

		const int id  = MerseneInt32() % keys.size();
		const int key = keys[id];
		downs[key]    = true;

		int modifier = Modifier_None;
		if (MerseneInt32() & 1) modifier |= Modifier_Shift;
		if ((MerseneInt32() & 3) == 0) modifier |= Modifier_Meta;

		// ui::Log("Scramble: id={} key={} modifier={} repeat={}", id, key, modifier, repeat);
		AiControls(camera, modifier, downs);
	}
}

int RubikCube::Serialize(fmt::memory_buffer& outString, const int depth, const int bounds, const bool addChildren) const
{
	int keyId = Mesh::Serialize(outString, depth, bounds & 1, false);
	if (keyId < 0) return keyId;

	WRITE_KEY_INT(cubeSize);

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void RubikCube::SetPhysics(PhysicsWorld* physics)
{
	if (!body) CreateShapeBody(physics, ShapeType_Box, 1.0f);

	for (auto& cubie : children)
	{
		auto mesh = Mesh::SharedPtr(cubie);
		mesh->CreateShapeBody(physics, ShapeType_Box, 1.0f);
		mesh->body->enabled = body->enabled;
	}
}

void RubikCube::ShowTable() const
{
	Mesh::ShowTable();
	ui::ShowTable({
	    { "cubeSize", std::to_string(cubeSize) },
	});
}
