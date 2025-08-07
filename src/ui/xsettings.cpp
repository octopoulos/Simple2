// xsettings.cpp
// @author octopoulos
// @version 2025-08-02

#include "stdafx.h"
#include "ui/xsettings.h"

static std::string settingsToml = "settings.ini";

extern const char* sAspectRatios[6];
extern const char* sFullScreens[3];
extern const char* sProjections[2];
extern const char* sRenderModes[4];
extern const char* sThemes[5];
extern const char* sVSyncs[3];

const char* sGames[] = { "Custom1", "Custom2", "Custom3" };

// global variable
XSettings xsettings;
#define XSETTINGS XSettings

// MAPPING
//////////

// clang-format off
static std::vector<Config> configs = {
	#include "game-config.inl"

	// [analysis]
	// 0
	X_STRING (XSettings, analysis, 1, appId , ""),
	X_ENUM   (XSettings, analysis, 1, gameId, 0, sGames),

	// [physics]
	X_BOOL   (XSettings, physics, 0, bulletDebug, false),
	X_BOOL   (XSettings, physics, 0, physPaused , false),

	// [render]
	X_FLOAT  (XSettings, render, 0, cameraSpeed, 10.0f, 1.0f, 100.0f),
	X_FLOAT  (XSettings, render, 0, distance   , 10.0f, 0.50f, 100.0f),
	X_BOOL   (XSettings, render, 0, instancing , true),
	X_FLOAT  (XSettings, render, 0, orthoZoom  , 1.0f, 0.001f, 10.0f),
	X_FLOAT  (XSettings, render, 0, zoomKb     , 10.0f, 1.0f, 100.0f),
	X_FLOAT  (XSettings, render, 0, zoomWheel  , 10.0f, 1.0f, 100.0f),

	// [ui]
	X_FLOAT  (XSettings, ui, 0, iconSize    , 64.0f, 8.0f, 256.0f),
	X_BOOL   (XSettings, ui, 0, nvidiaEnc   , false),
	X_STRINGS(XSettings, ui, 0, recentFiles , "", 6),
	X_BOOL   (XSettings, ui, 0, videoCapture, false),

	// [user]
	X_STRING (XSettings, user, 0, userEmail, ""),
	X_STRING (XSettings, user, 1, userHost , ""),
	X_CRYPT  (XSettings, user, 0, userPw   , ""),
};
// clang-format on

#include "game-settings.inl"
