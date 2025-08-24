// stdafx.h
// @author octopoulos
// @version 2025-08-20

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

#include "AI/common.h"  // DEV, MerseneUint, s, SUBCASE_FMT, THROW_RUNTIME, ui, uSSL
#include "AI/extract.h" // YYJSON_*
#include "AI/html.h"    // FillTemplate, ReplaceMetas
#include "AI/text.h"    // FastAtoi32, FastAtoi32i, Hex64, Printify, SplitStringView
#include "AI/time.h"    // CurrentDateTime, FormatDate, FromDate32, MakeDateTime, Now32, Now64, OffsetDateText, UnixTimestamp

#include "entry/entry.h"
#include "loaders/loader.h"

// clang-format off
inline btVector3    BxToBullet (const bx::Vec3& vec )  { return btVector3(vec.x, vec.y, vec.z); }
inline btVector3    GlmToBullet(const glm::vec3& vec)  { return btVector3(vec.x, vec.y, vec.z); }
inline btQuaternion GlmToBullet(const glm::quat& quat) { return btQuaternion(quat.x, quat.y, quat.z, quat.w); }
// clang-format on
