// Material.cpp
// @author octopoulos
// @version 2025-08-23

#include "stdafx.h"
#include "materials/Material.h"
//
#include "core/ShaderManager.h"
#include "loaders/writer.h"

Material::Material(std::string_view vsName, std::string_view fsName)
{
	LoadProgram(vsName, fsName);
}

void Material::LoadProgram(std::string_view vsName, std::string_view fsName)
{
	this->fsName = fsName;
	this->vsName = vsName;
	program      = GetShaderManager().LoadProgram(vsName, fsName);
}

int Material::Serialize(fmt::memory_buffer& outString, int depth, int bounds)
{
	if (bounds & 1) WRITE_CHAR('{');
	WRITE_INIT();
	WRITE_KEY_STRING(fsName);
	WRITE_KEY_STRING(vsName);
	if (bounds & 2) WRITE_CHAR('}');
	return keyId;
}
