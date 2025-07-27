// xsettings.cpp
// @author octopoulos
// @version 2025-07-23

#include "stdafx.h"
#include "xsettings.h"

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
	X_INT    (XSettings, analysis, 1, view  , 125, 0, 255),

	// [user]
	X_STRING (XSettings, user, 0, userEmail, ""),
	X_STRING (XSettings, user, 1, userHost , ""),
	X_CRYPT  (XSettings, user, 0, userPw   , ""),
};
// clang-format on

#include "game-settings.inl"
