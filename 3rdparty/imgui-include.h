// imgui-include.h
// @author octopoulos
// @version 2025-07-31
//
// just include the correct imgui version

#pragma once

#ifdef WITH_IMGUI_DOCKING
#	include "imgui-docking/imgui.h"
#	include "imgui-docking/imgui_internal.h"
#else
#	include "dear-imgui/imgui.h"
#	include "dear-imgui/imgui_internal.h"
#endif // WITH_IMGUI_DOCKING
