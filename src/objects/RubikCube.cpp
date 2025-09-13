// RubikCube.cpp
// @author octopoulos
// @version 2025-09-09

#include "stdafx.h"
#include "objects/RubikCube.h"
//
#include "core/common3d.h"             // BxToGlm
#include "entry/input.h"               // GetGlobalInput
#include "geometries/Geometry.h"       // uGeometry
#include "materials/MaterialManager.h" // GetMaterialManager
#include "loaders/writer.h"            // WRITE_KEY_xxx
#include "ui/ui.h"                     // ui::

struct RubikFace
{
	std::string color       = "";              // #ff0000
	std::string name        = "";              // blue, green, orange, red, white, yellow
	glm::vec3   normal      = glm::vec3(0.0f); // normal
	// computed
	glm::vec3   normalWorld = glm::vec3(0.0f); // rotated normal (world)
};

const std::vector<RubikFace> rubikStartFaces = {
	{ "", "yellow", {  0.0f,  1.0f,  0.0f } },
	{ "", "white" , {  0.0f, -1.0f,  0.0f } },
	{ "", "red"   , {  1.0f,  0.0f,  0.0f } },
	{ "", "orange", { -1.0f,  0.0f,  0.0f } },
	{ "", "green" , {  0.0f,  0.0f,  1.0f } },
	{ "", "blue"  , {  0.0f,  0.0f, -1.0f } },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// https://www.gancube.com/pages/3x3x3-cfop-guide-of-gancube
void RubikCube::Controls(const sCamera& camera, int modifier, const bool* downs, bool* ignores, const bool* keys)
{
	using namespace entry;

	static const USET_INT newIgnores = {
		Key::KeyB, Key::KeyD, Key::KeyE, Key::KeyF, Key::KeyL, Key::KeyM,
		Key::KeyR, Key::KeyS, Key::KeyU, Key::KeyX, Key::KeyY, Key::KeyZ,
	};

	const float angleInc = bx::toRad(90.0f);
	int         change   = 0;
	auto&       ginput   = GetGlobalInput();
	const bool  isShift  = (modifier & Modifier_Shift);

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

	ui::Log("FRONT: maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", frontMaxDot, frontMaxFace->name, frontVis, frontMinDot, frontMinFace->name, glm::dot(frontMinFace->normalWorld, backward));
	ui::Log("RIGHT: maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", rightMaxDot, rightMaxFace->name, rightVis, rightMinDot, rightMinFace->name, glm::dot(rightMinFace->normalWorld, backward));
	ui::Log("UP   : maxDot={:5.2f} ({:6}) {:5.2f} : minDot={:5.2f} ({:6}) {:5.2f}", upMaxDot   , upMaxFace->name   , upVis   , upMinDot   , upMinFace->name   , glm::dot(upMinFace->normalWorld   , backward));

	// 2) whole cube rotation
	// 2.a) X: front to up => rotation around X axis (right)
	if (GI_DOWN_REPEAT(Key::KeyX))
	{
		const auto& axis    = rightMaxFace->normalWorld;
		glm::quat   rotQuat = glm::angleAxis(angleInc * (isShift ? -1.0f : 1.0f), axis);
		quaternion          = glm::normalize(rotQuat * quaternion);
		change++;
	}

	// 2.b) Z: Bring right/front to up (opposite of X)
	if (GI_DOWN_REPEAT(Key::KeyZ))
	{
	// 	const float dir     = (rightVisible ? -1.0f : 1.0f) * (isShift ? -1.0f : 1.0f); // Opposite for right
	// 	const float deg     = dir * angleInc;
	// 	glm::quat   rotQuat = glm::angleAxis(bx::toRad(deg), cameraUp); // Around camera up
	// 	quaternion          = rotQuat * quaternion;
	// 	rotation            = glm::eulerAngles(quaternion);
	// 	irot[1]             = TO_INT(bx::toDeg(rotation.y));
	// 	change++;
	// 	ui::Log("Z rotation: deg={} (right visible: {})", deg, rightVisible);
	}

	// // // Y: Roll to bring horiz (up/down) to front (around horizontalViewDir)
	if (GI_DOWN_REPEAT(Key::KeyY))
	{
	// //     bool        horizVisible = upVisible || downVisible;  // Always true, but for logic
	// //     const float dir          = (upVisible ? 1.0f : -1.0f) * (isShift ? -1.0f : 1.0f);
	// //     const float deg          = dir * angleInc;
	// //     glm::quat   rotQuat      = glm::angleAxis(bx::toRad(deg), horizontalViewDir); // Around horizontal view
	// //     quaternion               = rotQuat * quaternion;
	// //     rotation                 = glm::eulerAngles(quaternion);
	// //     irot[2]                  = TO_INT(bx::toDeg(rotation.z)); // Snap roll
	// //     change++;
	// //     ui::Log("Y rotation: deg={} (up visible: {})", deg, upVisible);
	}

	// // 2) Face rotations (U/R/L/F/B/D/E/M/S): Layer-based, cube-local axes
	// struct FaceTurn
	// {
	// 	Key::Enum key;      // key
	// 	char      face;     // e.g., 'U'
	// 	int       axis;     // 0=X, 1=Y, 2=Z
	// 	int       dir;      // +1 or -1 for clockwise view
	// 	float     layerPos; // Position threshold (±1.0 outer, 0.0 middle)
	// };

	// const std::vector<FaceTurn> faceTurns = {
	// 	// Outer faces
	// 	{ Key::KeyU, 'U', 1, +1, +1.0f }, // Up: +Y (yellow)
	// 	{ Key::KeyD, 'D', 1, -1, -1.0f }, // Down: -Y (white)
	// 	{ Key::KeyR, 'R', 0, +1, +1.0f }, // Right: +X (red)
	// 	{ Key::KeyL, 'L', 0, -1, -1.0f }, // Left: -X (orange)
	// 	{ Key::KeyF, 'F', 2, +1, +1.0f }, // Front: +Z (green)
	// 	{ Key::KeyB, 'B', 2, -1, -1.0f }, // Back: -Z (blue)
	// 	// Middle slices
	// 	{ Key::KeyE, 'E', 1, -1, 0.0f  }, // Equator: middle Y (as D)
	// 	{ Key::KeyM, 'M', 0, -1, 0.0f  }, // Middle X (as L)
	// 	{ Key::KeyS, 'S', 2, -1, 0.0f  }  // Standing: middle Z (as B)
	// };

	// static std::unordered_map<Key::Enum, int64_t> lastPressTime; // For double-tap 180°

	// for (const auto& turn : faceTurns)
	// {
	// 	if (GI_DOWN_REPEAT(turn.key))
	// 	{
	// 		auto& lastTime = lastPressTime[turn.key];
	// 		bool  isDouble = (ginput.nowMs - lastTime < 500) && ginput.RepeatingKey(turn.key);
	// 		lastTime       = ginput.nowMs;

	// 		const int   numTurn = isDouble ? 2 : 1;
	// 		const int   rotDir  = isShift ? -numTurn : numTurn;
	// 		const float deg     = TO_FLOAT(rotDir * angleInc);

	// 		RotateFace(turn.axis, turn.dir, turn.layerPos, deg);
	// 		change++;

	// 		std::string suffix = isShift ? "'" : (isDouble ? "2" : "");
	// 		ui::Log("Turn: {}{} (axis={}, dir={}, layer={:.1f}, deg={})", turn.face, suffix, turn.axis, turn.dir, turn.layerPos, deg);
	// 	}
	// }

	// 3) apply changes
	if (change)
	{
		ui::Log("change={}", change);
		RotationFromQuaternion();
		UpdateLocalMatrix("Controls");
	}

	// 4) set ignored keys
	for (const int ignore : newIgnores)
		ignores[ignore] = true;
}

void RubikCube::CreateCubies()
{
	load = MeshLoad_Full;

	// create material for all cubies
	const auto cubieMaterial = GetMaterialManager().LoadMaterial("rubik_cubie", "vs_rubik", "fs_rubik");

	// define face colors (RGBA)
	const std::vector<glm::vec4> faceColors = {
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // +x (red)
		glm::vec4(1.0f, 0.7f, 0.0f, 1.0f), // -x (orange)
		glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), // +y (yellow)
		glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), // -y (white)
		glm::vec4(0.0f, 0.8f, 0.0f, 1.0f), // +z (green)
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), // -z (blue)
	};
	const glm::vec4 defaultColor(0.4f, 0.4f, 0.4f, 1.0f); // black/gray for inner faces

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
				const auto cubie = std::make_shared<Mesh>(cubieName, cubieGeometry, cubieMaterial);

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

void RubikCube::RotateFace(int axis, int dir, float layerThreshold, float deg)
{
	// Get cube's local rotation matrix from quaternion
	glm::mat3 localRot = glm::mat3_cast(quaternion);
	glm::vec3 rotAxis  = localRot[axis] * TO_FLOAT(dir); // e.g., local +Y for U
	rotAxis            = glm::normalize(rotAxis);

	// rotate only cubies in the target layer around cube center (0,0,0)
	const float tolerance = 0.1f; // for floating-point matching
	for (auto& child : children)
	{
		glm::vec3 localPos = child->position;
		bool      inLayer  = false;
		// clang-format off
		     if (axis == 0) inLayer = fabsf(localPos.x - layerThreshold) < tolerance; // X layer
		else if (axis == 1) inLayer = fabsf(localPos.y - layerThreshold) < tolerance; // Y
		else if (axis == 2) inLayer = fabsf(localPos.z - layerThreshold) < tolerance; // Z
		// clang-format on

		if (inLayer)
		{
			// apply rotation to position
			const float rad     = bx::toRad(deg);
			glm::quat   rotQuat = glm::angleAxis(rad, rotAxis);
			localPos            = rotQuat * localPos; // Rotate around center

			// snap to grid for precision (spacing=1.0f, positions like -1.0/0.0/+1.0)
			localPos = glm::floor(localPos * 2.0f + 0.5f) * 0.5f;

			// update cubie transform (no per-cubie rotation)
			glm::vec3          scale(1.0f);
			std::array<int, 3> zeroIrot = { 0, 0, 0 };
			child->ScaleIrotPosition(scale, zeroIrot, localPos);
		}
	}

	// propagate matrix updates
	UpdateWorldMatrix(true);
}

int RubikCube::Serialize(fmt::memory_buffer& outString, const int depth, const int bounds, const bool addChildren) const
{
	int keyId = Mesh::Serialize(outString, depth, bounds & 1, false);
	if (keyId < 0) return keyId;

	WRITE_KEY_INT(cubeSize);

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void RubikCube::ShowTable() const
{
	Mesh::ShowTable();
	ui::ShowTable({
	    { "cubeSize", std::to_string(cubeSize) },
	});
}
