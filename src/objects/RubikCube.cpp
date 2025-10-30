// RubikCube.cpp
// @author octopoulos
// @version 2025-10-26

#include "stdafx.h"
#include "objects/RubikCube.h"
//
#include "common/config.h"             // DEV_rubik
#include "core/common3d.h"             // BxToGlm
#include "entry/input.h"               // GetGlobalInput
#include "geometries/Geometry.h"       // uGeometry
#include "loaders/MeshLoader.h"        // MeshLoader
#include "loaders/writer.h"            // WRITE_KEY_xxx
#include "materials/MaterialManager.h" // GetMaterialManager
#include "ui/ui.h"                     // ui::
#include "ui/xsettings.h"              // xsettings

static const std::vector<glm::vec4> faceColors = {
	glm::vec4(0.85f, 0.85f, 0.85f, 1.00f), // gray
	glm::vec4(0.00f, 0.60f, 0.95f, 1.00f), // -z (blue)
	glm::vec4(0.00f, 0.85f, 0.00f, 1.00f), // +z (green)
	glm::vec4(1.10f, 0.70f, 0.00f, 1.00f), // -x (orange)
	glm::vec4(0.95f, 0.00f, 0.00f, 1.00f), // +x (red)
	glm::vec4(1.10f, 1.10f, 1.10f, 1.00f), // -y (white)
	glm::vec4(1.00f, 0.95f, 0.00f, 1.00f), // +y (yellow)
};

enum CubeColors_ : int
{
	Col_0 = 0,
	Col_B = 1,
	Col_G = 2,
	Col_O = 3,
	Col_R = 4,
	Col_W = 5,
	Col_Y = 6,
};

struct Cubie
{
	std::vector<int>   colors   = {};              ///< material colors
	std::array<int, 3> irot     = { 0, 0, 0 };     ///< initial rotation
	std::string        name     = "";              ///< edge: yellow-blue
	glm::vec3          normal   = glm::vec3(0.0f); ///< face normal
	int                start[3] = { 0, 0, 0 };     ///< starting position (-1, 0, 1)
};

// clang-format off
static const std::vector<Cubie> rubikCubies = {
	{ { Col_O, Col_B, Col_W }, {   0, 180,  90 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 0 0 -x-y-z
	{ { Col_W, Col_O }       , { 180, 180,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 0 1 -x-y
	{ { Col_W, Col_G, Col_O }, {   0,   0, 180 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 0 2 -x-y+z
	{ { Col_O, Col_B }       , {   0,  90,  90 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 1 0 -x-z
	{ { Col_O }              , {   0,   0,  90 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 1 1 -x
	{ { Col_O, Col_G }       , {  90, -90,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 1 2 -x+z
	{ { Col_Y, Col_B, Col_O }, {   0, 180,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 2 0 -x+y-z
	{ { Col_Y, Col_O }       , {   0, 180,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 2 1 -x+y
	{ { Col_Y, Col_O, Col_G }, {   0, -90,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 0 2 2 -x+y+z
	{ { Col_W, Col_B }       , {   0,  90, 180 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 0 0 -y-z
	{ { Col_W }              , { 180,   0,   0 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 0 1 -y
	{ { Col_W, Col_G }       , {   0, -90, 180 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 0 2 -y+z
	{ { Col_B }              , { -90,   0,   0 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 1 0 -z
	{ {}                     , {   0,   0,   0 }, "core"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 1 1
	{ { Col_G }              , {  90,   0,   0 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 1 2 +z
	{ { Col_Y, Col_B }       , {   0,  90,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 2 0 +y-z
	{ { Col_Y }              , {   0,   0,   0 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 2 1 +y
	{ { Col_Y, Col_G }       , {   0, -90,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 1 2 2 +y+z
	{ { Col_R, Col_W, Col_B }, {  90,  90,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 0 0 +x-y-z
	{ { Col_W, Col_R }       , { 180,   0,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 0 1 +x-y
	{ { Col_G, Col_W, Col_R }, {  90,   0,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 0 2 +x-y+z
	{ { Col_B, Col_R }       , { -90,   0,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 1 0 +x-z
	{ { Col_R }              , {   0,   0, -90 }, "center", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 1 1 +x
	{ { Col_G, Col_R }       , {  90,   0,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 1 2 +x+z
	{ { Col_Y, Col_R, Col_B }, {   0,  90,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 2 0 +x+y-z
	{ { Col_Y, Col_R }       , {   0,   0,   0 }, "edge"  , { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 2 1 +x+y
	{ { Col_Y, Col_G, Col_R }, {   0,   0,   0 }, "corner", { 0.0f, 1.0f, 0.0f }, { 0, 1, 0 } }, // 2 2 2 +x+y+z
};
// clang-format on

static const std::vector<RubikFace> rubikStartFaces = {
	{ "", "yellow", {  0.0f,  1.0f,  0.0f } },
	{ "", "white" , {  0.0f, -1.0f,  0.0f } },
	{ "", "red"   , {  1.0f,  0.0f,  0.0f } },
	{ "", "orange", { -1.0f,  0.0f,  0.0f } },
	{ "", "green" , {  0.0f,  0.0f,  1.0f } },
	{ "", "blue"  , {  0.0f,  0.0f, -1.0f } },
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void RubikCube::AiControls(const sCamera& camera, int modifier, const bool* downs, bool isQueue)
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
		ui::Log("FRONT: maxDot=%5.2f (%s) %5.2f : minDot=%5.2f (%s) %5.2f", frontMaxDot, Cstr(frontMaxFace->name), frontVis, frontMinDot, Cstr(frontMinFace->name), glm::dot(frontMinFace->normalWorld, backward));
		ui::Log("RIGHT: maxDot=%5.2f (%s) %5.2f : minDot=%5.2f (%s) %5.2f", rightMaxDot, Cstr(rightMaxFace->name), rightVis, rightMinDot, Cstr(rightMinFace->name), glm::dot(rightMinFace->normalWorld, backward));
		ui::Log("UP   : maxDot=%5.2f (%s) %5.2f : minDot=%5.2f (%s) %5.2f", upMaxDot   , Cstr(upMaxFace->name)   , upVis   , upMinDot   , Cstr(upMinFace->name)   , glm::dot(upMinFace->normalWorld   , backward));
	}

	// 2) whole cube rotation
	int angle = 90;
	if (modifier & Modifier_Shift) angle *= -1;
	if (modifier & Modifier_Meta) angle *= 2;

	const int step = 0;

	auto keyRotateCube = [&](int key, const RubikFace* face) {
		if (!isDirty && GI_DOWN(key)) RotateCube(face, angle, GlobalInput::EncodeKey(key, modifier), isQueue);
	};

	// clang-format off
	keyRotateCube(Key::KeyX, rightMaxFace);
	keyRotateCube(Key::KeyY, upMaxFace   );
	keyRotateCube(Key::KeyZ, frontMaxFace);
	// clang-format off

	// 3) handle layer rotations
	auto keyRotateLayer = [&](int key, const RubikFace* face) {
		if (!isDirty && GI_DOWN(key)) RotateLayer(face, angle, GlobalInput::EncodeKey(key, modifier), isQueue);
	};

	// clang-format off
	keyRotateLayer(Key::KeyB, frontMinFace);
	keyRotateLayer(Key::KeyD, upMinFace   );
	keyRotateLayer(Key::KeyF, frontMaxFace);
	keyRotateLayer(Key::KeyL, rightMinFace);
	keyRotateLayer(Key::KeyR, rightMaxFace);
	keyRotateLayer(Key::KeyU, upMaxFace   );
	// clang-format on

	// 4) extras
	if (GI_DOWN(Key::KeyI)) Explode();

	// 5) apply changes
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
		Key::KeyI,
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

	auto&     ginput = GetGlobalInput();
	const int step   = 0;

	if (nextKeys.size())
	{
		const int encoded = nextKeys.front();
		nextKeys.pop_front();

		const auto& [key, modifier] = ginput.DecodeKey(encoded);

		bool downs[256] = {};
		downs[key]      = true;

		AiControls(camera, modifier, downs, true);
	}

	// 1) scramble?
	if (GI_REPEAT_RUBIK(Key::KeyK))
		Scramble(camera, 1);
	// 2) AI controls
	else
		AiControls(camera, modifier, downs, false);

	// 3) set ignored keys
	for (const int ignore : newIgnores)
		ignores[ignore] = true;
}

void RubikCube::Initialize()
{
	load = MeshLoad_Full;

	// total size of each cubie (edge length)
	const float spacing = 1.0f;                // distance between cubie centers
	const int   S       = cubeSize - 1;        // shortcut
	const float offset  = -S * spacing / 2.0f; // center the cube at origin

	// create n^3 cubies
	int n = -1;
	for (int x = 0; x < cubeSize; ++x)
	{
		for (int y = 0; y < cubeSize; ++y)
		{
			for (int z = 0; z < cubeSize; ++z)
			{
				// name each cubie for identification
				const auto cubieName = FormatStr("Cubie_%d_%d_%d", x, y, z);

				++n;
				const auto& cubie = rubikCubies[n];

				if (cubie.name.size()) // x == 1 && y == S && z == 1)
				{
					const auto splits = SplitStringView(cubie.name, ':');
					auto       mesh   = MeshLoader::LoadModelFull(cubieName, FormatStr("rubik/%s", Cstr(splits[0])));

					for (auto& group : mesh->groups)
					{
						if (group.material)
						{
							const auto& color = group.material->baseColorFactor;
							ui::Log("Group.material: [%s] %f %f %f %f", Cstr(group.material->name), color.r, color.g, color.b, color.a);

							if (const auto splits = SplitStringView(group.material->name, ':'); splits.size() >= 3 && splits[1] == "Face")
							{
								if (const int cid = FastAtoi32i(splits[2]); cid >= 1 && cid <= cubie.colors.size())
								{
									group.material = group.material->Clone();
									group.material->baseColorFactor = faceColors[cubie.colors[cid - 1]];
								}
							}
						}
					}

					// position the cubie in the 3D grid
					const glm::vec3 position(
						offset + x * spacing,
						offset + y * spacing,
						offset + z * spacing);

					const float     radius = 52.0f;
					const glm::vec3 scale(radius);

					mesh->ScaleIrotPosition(scale, cubie.irot, position);
					AddChild(mesh);
				}
			}
		}
	}

	ui::Log("RubikCube: Created %d cubies for %dx%dx%d cube", cubeSize * cubeSize * cubeSize, cubeSize, cubeSize, cubeSize);

	// update world matrices for all children
	UpdateWorldMatrix(true);
}

void RubikCube::RotateCube(const RubikFace* face, int angle, int key, bool isQueue)
{
	// 1) handle non completed interpolation
	const int change = CompleteInterpolation(false, "RotateCube");
	ui::Log("RotateCube: %d", change);
	if (change)
	{
		QueueKey(key, isQueue);
		return;
	}

	// 2) new interpolation
	{
		const glm::vec3& axis    = face->normalWorld;
		const glm::quat  rotQuat = glm::angleAxis(bx::toRad(angle), axis);

		interval    = GetInterval(true);
		quaternion1 = quaternion;
		quaternion2 = glm::normalize(rotQuat * quaternion);
		quatTs      = Nowd();
		isDirty     = true;
	}
}

void RubikCube::RotateLayer(const RubikFace* face, int angle, int key, bool isQueue)
{
	// 1) stop current interpolation
	const int change = CompleteInterpolation(false, "RotateLayer");
	ui::Log("RotateLayer: %d", change);
	if (change)
	{
		QueueKey(key, isQueue);
		return;
	}

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

		interval = GetInterval(true);

		for (auto& cubie : cubies)
		{
			// rotate local position around local axis (0,0,0) + snap to grid
			{
				glm::vec3 newLocalPos = rotQuat * cubie->position;
				newLocalPos = glm::floor(newLocalPos * 2.0f + 0.5f) * 0.5f;

				cubie->axis1       = glm::identity<glm::quat>();
				cubie->axis2       = rotQuat;
				cubie->interval    = interval;
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
	else ui::Log("\ncubies=%zu\n", cubies.size());
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

		// ui::Log("Scramble: id=%d key=%d modifier=%d repeat=%d", id, key, modifier, repeat);
		AiControls(camera, modifier, downs, false);
	}
}

int RubikCube::Serialize(std::string& outString, const int depth, const int bounds, const bool addChildren) const
{
	int keyId = Mesh::Serialize(outString, depth, bounds & 1, false);
	if (keyId < 0) return keyId;

	WRITE_KEY_INT(cubeSize);

	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void RubikCube::SetPhysics()
{
	if (!body) CreateShapeBody(ShapeType_Box, 1.0f);

	for (auto& cubie : children)
	{
		if (auto mesh = Mesh::SharedPtr(cubie); !mesh->body)
		{
			mesh->CreateShapeBody(ShapeType_Box, 1.0f);
			mesh->ActivatePhysics(false);
		}
	}
}

void RubikCube::ShowInfoTable(bool showTitle) const
{
	Mesh::ShowInfoTable(showTitle);
	if (showTitle) ImGui::TextUnformatted("RubikCube");

	// clang-format off
	ui::ShowTable({
		{ "cubeSize", std::to_string(cubeSize) },
	});
	// clang-format on
}
