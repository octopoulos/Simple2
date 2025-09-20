// Geometry.cpp
// @author octopoulos
// @version 2025-09-16

#include "stdafx.h"
#include "geometries/Geometry.h"
//
#include "loaders/writer.h" // WRITE_INIT, WRITE_KEY_xxx
#include "ui/ui.h"          // ui::

// clang-format off
static const UMAP_INT_STR geometryTypeNames = {
	{ GeometryType_None        , "None"         },
	{ GeometryType_Box         , "Box"          },
	{ GeometryType_Capsule     , "Capsule"      },
	{ GeometryType_Cone        , "Cone"         },
	{ GeometryType_Cylinder    , "Cylinder"     },
	{ GeometryType_Dodecahedron, "Dodecahedron" },
	{ GeometryType_Icosahedron , "Icosahedron"  },
	{ GeometryType_Octahedron  , "Octahedron"   },
	{ GeometryType_Plane       , "Plane"        },
	{ GeometryType_Sphere      , "Sphere"       },
	{ GeometryType_Tetrahedron , "Tetrahedron"  },
	{ GeometryType_Torus       , "Torus"        },
	{ GeometryType_TorusKnot   , "TorusKnot"    },
};
// clang-format on

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GEOMETRY
///////////

int Geometry::Serialize(fmt::memory_buffer& outString, int depth, int bounds) const
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	WRITE_KEY_STRING(args);
	WRITE_KEY_STRING2("type", GeometryName(type));
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}

void Geometry::ShowInfoTable(bool showTitle) const
{
	if (showTitle) ImGui::TextUnformatted("Geometry");

	// clang-format off
	ui::ShowTable({
		{ "aabb"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", aabb.x(), aabb.y(), aabb.z()) },
		{ "args"  , args                                                              },
		{ "dims"  , fmt::format("{:.2f}:{:.2f}:{:.2f}", dims.x(), dims.y(), dims.z()) },
		{ "radius", std::to_string(radius)                                            },
		{ "type"  , std::to_string(type)                                              },
	});
	// clang-format on
}

void Geometry::ShowSettings(bool isPopup, int show)
{
	int mode = 3;
	if (isPopup) mode |= 4;

	ui::AddInputText(mode, ".args", "Args", 256, 0, &args);
	ui::AddDragFloat(mode, ".radius", "Radius", &radius);
	ui::AddSliderInt(mode, ".type", "Type", &type, 1, GeometryType_None, GeometryType_Count - 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

uGeometry CreateAnyGeometry(int type, std::string_view sargs)
{
	// random
	if (type == GeometryType_None)
		type = MerseneInt32(GeometryType_Box, GeometryType_Count - 1);

	const VEC_VIEW args   = SplitStringView(sargs, ' ', false);
	int            argId  = 0;
	const int      numArg = TO_INT(args.size());

	// helper lambdas to parse args or fallback
	// clang-format off
	auto nextb = [&](bool  def) -> bool  { return (argId < numArg) ? !!FastAtoi32i(args[argId++]) : def; };
	auto nextf = [&](float def) -> float { return (argId < numArg) ?   FastAtof   (args[argId++]) : def; };
	auto nexti = [&](int   def) -> int   { return (argId < numArg) ?   FastAtoi32i(args[argId++]) : def; };
	// clang-format on

	// clang-format off
	switch (type)
	{
	case GeometryType_Box:
		return CreateBoxGeometry(
			nextf(1.0f), // width
			nextf(1.0f), // height
			nextf(1.0f), // depth
			nexti(1),    // widthSegments
			nexti(1),    // heightSegments
			nexti(1)     // depthSegments
		);
	case GeometryType_Capsule:
		return CreateCapsuleGeometry(
			nextf(1.0f), // radius
			nextf(1.0f), // height
			nexti(8),    // capSegments
			nexti(16),   // radialSegments
			nexti(1)     // heightSegments
		);
	case GeometryType_Cone:
		return CreateConeGeometry(
			nextf(1.0f),    // radius
			nextf(1.0f),    // height
			nexti(32),      // radialSegments
			nexti(1),       // heightSegments
			nextb(false),   // openEnded
			nextf(0.0f),    // thetaStart
			nextf(bx::kPi2) // thetaLength
		);
	case GeometryType_Cylinder:
		return CreateCylinderGeometry(
			nextf(1.0f),    // radiusTop
			nextf(1.0f),    // radiusBottom
			nextf(1.0f),    // height
			nexti(32),      // radialSegments
			nexti(1),       // heightSegments
			nextb(false),   // openEnded
			nextf(0.0f),    // thetaStart
			nextf(bx::kPi2) // thetaLength
		);
	case GeometryType_Dodecahedron:
		return CreateDodecahedronGeometry(
			nextf(1.0f), // radius
			nexti(0)     // detail
		);
	case GeometryType_Icosahedron:
		return CreateIcosahedronGeometry(
			nextf(1.0f), // radius
			nexti(0)     // detail
		);
	case GeometryType_Octahedron:
		return CreateOctahedronGeometry(
			nextf(1.0f), // radius
			nexti(0)     // detail
		);
	case GeometryType_Plane:
		return CreatePlaneGeometry(
			nextf(1.0f), // width
			nextf(1.0f), // height
			nexti(1),    // widthSegments
			nexti(1)     // heightSegments
		);
	case GeometryType_Sphere:
		return CreateSphereGeometry(
			nextf(1.0f),     // radius
			nexti(32),       // widthSegments
			nexti(16),       // heightSegments
			nextf(0.0f),     // phiStart
			nextf(bx::kPi2), // phiLength
			nextf(0.0f),     // thetaStart
			nextf(bx::kPi)   // thetaLength
		);
	case GeometryType_Tetrahedron:
		return CreateTetrahedronGeometry(
			nextf(1.0f), // radius
			nexti(0)     // detail
		);
	case GeometryType_Torus:
		return CreateTorusGeometry(
			nextf(1.0f),    // radius
			nextf(0.4f),    // tube
			nexti(12),      // radialSegments
			nexti(48),      // tubularSegments
			nextf(bx::kPi2) // arc
		);
	// uGeometry CreateTorusKnotGeometry(float radius = 1.0f, float tube = 0.4f, int tubularSegments = 64, int radialSegments = 8, int p = 2, int q = 3);
	case GeometryType_TorusKnot:
		return CreateTorusKnotGeometry(
			nextf(1.0f), // radius
			nextf(0.4f), // tube
			nexti(64),   // tubularSegments
			nexti(8),    // radialSegments
			nexti(2),    // p
			nexti(3)     // q
		);
	}
	// clang-format on

	return nullptr;
}

std::string GeometryName(int type)
{
	return FindDefault(geometryTypeNames, type, "???");
}

int GeometryType(std::string_view name)
{
	static UMAP_STR_INT geometryNameTypes;
	if (geometryNameTypes.empty())
	{
		for (const auto& [type, name] : geometryTypeNames)
			geometryNameTypes[name] = type;
	}

	return FindDefault(geometryNameTypes, name, GeometryType_None);
}
