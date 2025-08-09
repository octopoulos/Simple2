// Material.cpp
// @author octopoulos
// @version 2025-08-04

#include "stdafx.h"
#include "materials/Material.h"
//
#include "loaders/writer.h"

int Material::Serialize(fmt::memory_buffer& outString, int bounds)
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}
