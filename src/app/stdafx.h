// stdafx.h
// @author octopoulos
// @version 2025-09-27

#pragma once

#include "AI/stdafx.h"

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

#ifdef __EMSCRIPTEN__
#	include <emscripten.h>
#	include <emscripten/html5.h>
#endif // __EMSCRIPTEN__

using namespace std::literals;

#include "AI/common.h"  // DEV, MerseneUint, NormalizeFilename, RelativeName, s, SUBCASE_FMT, THROW_RUNTIME, ui, uSSL
#include "AI/extract.h" // YYJSON_*
#include "AI/html.h"    // FillTemplate, ReplaceMetas
#include "AI/text.h"    // Cstr, FastAtoi32, FastAtoi32i, Format, FormatStr, Hex64, Printify, SplitStringView
#include "AI/time.h"    // CurrentDateTime, FormatDate, FromDate32, MakeDateTime, Now32, Now64, OffsetDateText, UnixTimestamp

#include "entry/entry.h"
#include "loaders/loader.h"

#define BGFX_DESTROY(x)                         \
	do                                          \
	{                                           \
		if (bgfx::isValid(x)) bgfx::destroy(x); \
	}                                           \
	while (0)

#define DESTROY_GUARD()    \
	if (destroyed) return; \
	destroyed = true
