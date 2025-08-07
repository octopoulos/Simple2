// RubikCube.h
// @author octopoulos
// @version 2025-08-03

#pragma once

#include "objects/Object3d.h"

class RubikCube : public Object3d
{
public:
	RubikCube()
	    : name("RubikCube")
		, type(ObjectType_RubikCube)
	{
	}

	~RubikCube() = default;
};
