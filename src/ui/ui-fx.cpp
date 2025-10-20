// ui-fx.cpp
// @author octopoulos
// @version 2025-10-16
//
// https://github.com/ocornut/imgui/issues/3606

#include "stdafx.h"
#include "ui/ui-fx.h"

namespace ui
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

void Fx_NullFunc(ImDrawList* drawList, ImVec2 topLeft, ImVec2 bottomRight, ImVec2 size, ImVec4 mouse, float time)
{
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// INTERFACE
////////////

// Singleton for fxFunctions and fxNames
struct FxRegistry
{
	MAP_STR<FxFunc> fxFunctions;
	VEC_STR         fxNames;

	static FxRegistry& Get()
	{
		static FxRegistry instance;
		return instance;
	}
};

void AddFxFunction(const std::string& name, FxFunc func)
{
	ui::Log("AddFxFunction: %s", Cstr(name));
	auto& registry = FxRegistry::Get();
	registry.fxFunctions[name] = func;
}

void CreateFxNames()
{
	auto& registry = FxRegistry::Get();
	registry.fxNames.clear();
	for (const auto& it : registry.fxFunctions) registry.fxNames.push_back(it.first);
}

FxFunc GetFxFunction(std::string_view name)
{
	auto& registry = FxRegistry::Get();
	if (const auto& it = registry.fxFunctions.find(name); it != registry.fxFunctions.end())
		return it->second;
	return Fx_NullFunc;
}

MAP_STR<FxFunc> GetFxFunctions()
{
	auto& registry = FxRegistry::Get();
	return registry.fxFunctions;
}

VEC_STR GetFxNames()
{
	auto& registry = FxRegistry::Get();
	return registry.fxNames;
}

} // namespace ui
