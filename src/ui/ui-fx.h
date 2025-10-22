// ui-fx.h
// @author octopoulos
// @version 2025-10-18

#pragma once

#include "ui/ui.h"
//
#include "imgui.h"          // ImGui::
#include "imgui-include.h"  //
#include "imgui_internal.h" // IM_PI

namespace ui
{

#define FX_REGISTER(name)                        \
	namespace                                    \
	{                                            \
	struct FxRegistrar_##name                    \
	{                                            \
		FxRegistrar_##name()                     \
		{                                        \
			ui::AddFxFunction(#name, Fx_##name); \
		}                                        \
	};                                           \
	static FxRegistrar_##name registrar_##name;  \
	}

using FxFunc = void (*)(ImDrawList* drawList, ImVec2 topLeft, ImVec2, ImVec2 size, ImVec4, float time);

/// Add a new FX function
void AddFxFunction(const std::string& name, FxFunc func);

/// Create fxNames from fxFunctions
void CreateFxNames();

/// Get one FX function
FxFunc GetFxFunction(std::string_view name);

/// Get all FX functions
MAP_STR<FxFunc> GetFxFunctions();

/// Get all FX names
VEC_STR GetFxNames();

} // namespace ui
