// stdafx.h
// @author octopoulos
// @version 2025-07-05

#pragma once

#include "AI/stdafx.h"

#include <numbers>   // pi
#include <stdexcept> // std::runtime_error

#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

#include <bx/bx.h>
#include <bx/file.h>
#include <bx/filepath.h>
#include <bx/math.h>
#include <bx/readerwriter.h>
#include <bx/string.h>
#include <bx/timer.h>

#include <SDL3/SDL.h>

#ifdef __EMSCRIPTEN__
#	include <emscripten.h>
#	include <emscripten/html5.h>
#endif

using namespace std::literals;

#include "AI/common.h"  // DEV, MerseneUint, s, SUBCASE_FMT, THROW_RUNTIME, ui, uSSL
#include "AI/extract.h" // YYJSON_*
#include "AI/html.h"    // FillTemplate, ReplaceMetas
#include "AI/text.h"    // FastAtoi32, FastAtoi32i, Hex64, Printify, SplitStringView
#include "AI/time.h"    // CurrentDateTime, FormatDate, FromDate32, MakeDateTime, Now32, Now64, OffsetDateText, UnixTimestamp

#include "entry/entry.h"
#include "loader.h"

#include "engine/Geometry.h"
#include "engine/Material.h"
#include "engine/Object3d.h"
#include "engine/Mesh.h"
#include "engine/Camera.h"
#include "engine/Scene.h"
