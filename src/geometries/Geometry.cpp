// Geometry.cpp
// @author octopoulos
// @version 2025-07-30

#include "stdafx.h"
#include "geometries/Geometry.h"

// clang-format off
static const UMAP_INT_STR GEOMETRY_NAMES = {
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

uGeometry CreateAnyGeometry(int type)
{
	// random
	if (type == GeometryType_None)
		type = MerseneInt32(GeometryType_Box, GeometryType_Count - 1);

	// clang-format off
	switch (type)
	{
	case GeometryType_Box:          return CreateBoxGeometry();
	case GeometryType_Capsule:      return CreateCapsuleGeometry();
	case GeometryType_Cone:         return CreateConeGeometry();
	case GeometryType_Cylinder:     return CreateCylinderGeometry();
	case GeometryType_Dodecahedron: return CreateDodecahedronGeometry();
	case GeometryType_Icosahedron:  return CreateIcosahedronGeometry();
	case GeometryType_Octahedron:   return CreateOctahedronGeometry();
	case GeometryType_Plane:        return CreatePlaneGeometry();
	case GeometryType_Sphere:       return CreateSphereGeometry();
	case GeometryType_Tetrahedron:  return CreateTetrahedronGeometry();
	case GeometryType_Torus:        return CreateTorusGeometry();
	case GeometryType_TorusKnot:    return CreateTorusKnotGeometry();
	}
	// clang-format on

	return nullptr;
}

std::string GeometryName(int type)
{
	return FindDefault(GEOMETRY_NAMES, type, "???");
}
