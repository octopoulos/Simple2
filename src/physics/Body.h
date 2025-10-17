// Body.h
// @author octopoulos
// @version 2025-10-13

#pragma once

enum ShapeTypes_ : int
{
	ShapeType_None         = 0,  ///< btEmptyShape
	ShapeType_Box          = 1,  ///< btBox2dShape
	ShapeType_Box2d        = 2,  ///< btBox2dShape (default: AABB)
	ShapeType_BoxObb       = 3,  ///< btBox2dShape (OBB)
	ShapeType_Capsule      = 4,  ///< btCapsuleShape
	ShapeType_Compound     = 5,  ///< btCompoundShape
	ShapeType_Cone         = 6,  ///< btConeShape
	ShapeType_Convex2d     = 7,  ///< btConvex2dShape
	ShapeType_ConvexHull   = 8,  ///< btConvexHullShape
	ShapeType_Cylinder     = 9,  ///< btCylinderShape
	ShapeType_Plane        = 10, ///< btStaticPlaneShape
	ShapeType_Sphere       = 11, ///< btSphereShape (default bounding)
	ShapeType_Terrain      = 12, ///< btHeightfieldTerrainShape
	ShapeType_TriangleMesh = 13, ///< btBvhTriangleMeshShape
	//
	ShapeType_Count,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// BODY
///////

using wMesh = std::weak_ptr<class Mesh>;

class Body
{
public:
	btRigidBody*      body      = nullptr;                    ///< rigid body
	btVector4         dims      = { 0.0f, 0.0f, 0.0f, 0.0f }; ///< shape dims()
	bool              enabled   = true;                       ///< physics enabled
	btVector3         inertia   = { 0.0f, 0.0f, 0.0f };       ///< inertia
	float             mass      = 0.0f;                       ///< mass
	wMesh             meshWeak  = {};                         ///< parent mesh
	btCollisionShape* shape     = nullptr;                    ///< shape
	int               shapeType = ShapeType_None;             ///< ShapeTypes_

public:
	Body() = default;

	~Body() { Destroy(); }

	/// Activate/deactivate physical body
	void Activate(bool activate);

	/// Create a body after a shape has been created
	void CreateBody(float _mass, const btVector3& pos, const btQuaternion& quat);

	/// Create a collision shape, before the body
	void CreateShape(int type, const btVector4& newDims = { 0.0f, 0.0f, 0.0f, 0.0f });

	/// Destroy shape then body
	void Destroy();

	/// Destroy body + resets the pointer
	void DestroyBody();

	/// Destroy shape + resets the pointer
	void DestroyShape();

	/// Serialize for JSON output
	int Serialize(std::string& outString, int depth, int bounds = 3) const;

	/// Show info table in ImGui
	void ShowInfoTable(bool showTitle = true) const;

	/// Show settings in ImGui
	/// @param show: ShowObjects_
	void ShowSettings(bool isPopup, int show);
};

using uBody = std::unique_ptr<Body>;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Find the most appropriate body shape for a given geometry type
int GeometryShape(int geometryType, bool hasMass = true, int detail = 0);

/// Convert shape type: int to string
std::string ShapeName(int type);

/// Convert shape type: string to int
int ShapeType(std::string_view name);
