// Geometry.h
// @author octopoulos
// @version 2025-07-29

#pragma once

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// GEOMETRY
///////////

struct PosNormalUV
{
	float px, py, pz;
	float nx, ny, nz;
	float u, v;
};

class Geometry
{
public:
	bgfx::IndexBufferHandle  ibh = BGFX_INVALID_HANDLE; ///< indices
	std::vector<uint16_t>    indices;                   ///< used by ConvexHull + TriangleMesh
	bgfx::VertexBufferHandle vbh = BGFX_INVALID_HANDLE; ///< vertices
	std::vector<PosNormalUV> vertices;                  ///< used by ConvexHull + TriangleMesh

	// shape
	btVector3 aabb   = { 1.0f, 1.0f, 1.0f }; ///< aabb half extents
	btVector3 dims   = { 1.0f, 1.0f, 1.0f }; ///< dims for CreateShape (ex: radius, height)
	float     radius = 1.0f;                 ///< bounding sphere

	Geometry() = default;

	Geometry(bgfx::VertexBufferHandle vbh, bgfx::IndexBufferHandle ibh, const btVector3& aabb, const btVector3& dims, float radius, std::vector<PosNormalUV>&& vertices = {}, std::vector<uint16_t>&& indices = {})
	    : vbh(vbh)
	    , ibh(ibh)
		, aabb(aabb)
	    , dims(dims)
	    , radius(radius)
		, vertices(std::move(vertices))
		, indices(std::move(indices))
	{
	}

	~Geometry()
	{
		if (bgfx::isValid(ibh)) bgfx::destroy(ibh);
		if (bgfx::isValid(vbh)) bgfx::destroy(vbh);
	}
};

using uGeometry = std::shared_ptr<Geometry>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Constructs a new box geometry
/// @param width: length of the edges parallel to the X axis
/// @param height: length of the edges parallel to the Y axis
/// @param depth: length of the edges parallel to the Z axis
/// @param widthSegments: number of segmented rectangular faces along the width of the sides
/// @param heightSegments: number of segmented rectangular faces along the height of the sides
/// @param depthSegments: number of segmented rectangular faces along the depth of the sides
uGeometry CreateBoxGeometry(float width = 1.0f, float height = 1.0f, float depth = 1.0f, int widthSegments = 1, int heightSegments = 1, int depthSegments = 1);

/// Constructs a new capsule geometry
/// @param radius: radius of the capsule
/// @param height: height of the middle section
/// @param capSegments: number of curve segments used to build each cap
/// @param radialSegments: number of segmented faces around the circumference of the capsule, min 3
/// @param heightSegments: number of rows of faces along the height of the middle section, min 1
uGeometry CreateCapsuleGeometry(float radius = 1.0f, float height = 1.0f, int capSegments = 4, int radialSegments = 8, int heightSegments = 1);

/// Constructs a new cone geometry
/// @param radius: radius of the cone base
/// @param height: height of the cone
/// @param radialSegments: number of segmented faces around the circumference of the cone
/// @param heightSegments: number of rows of faces along the height of the cone
/// @param openEnded: whether the base of the cone is open or capped
/// @param thetaStart: start angle for first segment, in radians
/// @param thetaLength: central angle, often called theta, of the circular sector, in radians
uGeometry CreateConeGeometry(float radius = 1.0f, float height = 1.0f, int radialSegments = 32, int heightSegments = 1, bool openEnded = false, float thetaStart = 0.0f, float thetaLength = bx::kPi2);

/// Constructs a new cylinder geometry
/// @param radiusTop: radius at top of the cylinder
/// @param radiusBottom: radius at bottom of the cylinder
/// @param height: cylinder height
/// @param radialSegments: number of faces around circumference
/// @param heightSegments: number of rows along height
/// @param openEnded: true if caps are not generated
/// @param thetaStart: start angle in radians
/// @param thetaLength: sweep angle in radians
uGeometry CreateCylinderGeometry(float radiusTop = 1.0f, float radiusBottom = 1.0f, float height = 1.0f, int radialSegments = 32, int heightSegments = 1, bool openEnded = false, float thetaStart = 0.0f, float thetaLength = bx::kPi2);

/// Constructs a new dodecahedron geometry
/// @param radius: radius of the dodecahedron
/// @param detail: setting this to a value greater than `0` adds vertices making it no longer a dodecahedron
uGeometry CreateDodecahedronGeometry(float radius = 1.0f, int detail = 0);

/// Constructs a new icosahedron geometry
/// @param radius: radius of the icosahedron
/// @param detail: setting this to a value greater than `0` adds vertices making it no longer a icosahedron
uGeometry CreateIcosahedronGeometry(float radius = 1.0f, int detail = 0);

/// Constructs a new octahedron geometry
/// @param radius: radius of the octahedron
/// @param detail: setting this to a value greater than `0` adds vertices making it no longer a octahedron
uGeometry CreateOctahedronGeometry(float radius = 1.0f, int detail = 0);

/// Constructs a new plane geometry
/// @param width: width along the X axis
/// @param height: height along the Y axis
/// @param widthSegments: number of segments along the X axis
/// @param heightSegments: number of segments along the Y axis
uGeometry CreatePlaneGeometry(float width = 1.0f, float height = 1.0f, int widthSegments = 1, int heightSegments = 1);

/// Constructs a new polyhedron geometry
/// @param vertices: flat array of vertices describing the base shape
/// @param indices: flat array of indices describing the base shape
/// @param radius: radius of the shape
/// @param detail: how many levels to subdivide the geometry
uGeometry CreatePolyhedronGeometry(const float* vertices, int vertexCount, const uint16_t* indices, int indexCount, float radius, int detail);

/// Constructs a new sphere geometry
/// @param radius: sphere radius
/// @param widthSegments: number of horizontal segments, min 3
/// @param heightSegments: number of vertical segments, min 2
/// @param phiStart: horizontal starting angle in radians
/// @param phiLength: horizontal sweep angle size
/// @param thetaStart: vertical starting angle in radians
/// @param thetaLength: vertical sweep angle size
uGeometry CreateSphereGeometry(float radius = 1.0f, int widthSegments = 32, int heightSegments = 16, float phiStart = 0.0f, float phiLength = bx::kPi2, float thetaStart = 0.0f, float thetaLength = bx::kPi);

/// Constructs a new tetrahedron geometry
/// @param radius: radius of the tetrahedron
/// @param detail: setting this to a value greater than `0` adds vertices making it no longer a octahedron
uGeometry CreateTetrahedronGeometry(float radius = 1.0f, int detail = 0);

/// Constructs a new torus geometry
/// @param radius: distance from torus center to tube center
/// @param tube: radius of the tube
/// @param radialSegments: number of segments along tube cross-section
/// @param tubularSegments: number of segments around the torus ring
/// @param arc: sweep angle in radians (default 2π = full circle)
uGeometry CreateTorusGeometry(float radius = 1.0f, float tube = 0.4f, int radialSegments = 12, int tubularSegments = 48, float arc = bx::kPi2);

/// Constructs a new torus knot geometry
/// @param radius: radius of the torus knot
/// @param tube: radius of the tube
/// @param tubularSegments: number of tubular segments
/// @param radialSegments: number of radial segments
/// @param p: how many times the geometry winds around its axis of rotational symmetry
/// @param q: how many times the geometry winds around a circle in the interior of the torus
uGeometry CreateTorusKnotGeometry(float radius = 1.0f, float tube = 0.4f, int tubularSegments = 64, int radialSegments = 8, int p = 2, int q = 3);
