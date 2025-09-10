// RubikCube.cpp
// @author octopoulos
// @version 2025-09-05

#include "stdafx.h"
#include "objects/RubikCube.h"
//
#include "geometries/Geometry.h"       // uGeometry
#include "materials/MaterialManager.h" // GetMaterialManager
#include "loaders/writer.h"            // WRITE_KEY_xxx
#include "ui/ui.h"                     // ui::

void RubikCube::CreateCubies()
{
	load = MeshLoad_Full;

	// create material for all cubies
	const auto cubieMaterial = GetMaterialManager().LoadMaterial("rubik_cubie", "vs_rubik", "fs_rubik");

	// define face colors (RGBA)
	const std::vector<glm::vec4> faceColors = {
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f), // +x (red)
		glm::vec4(1.0f, 0.5f, 0.0f, 1.0f), // -x (orange)
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
