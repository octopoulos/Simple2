// xsettings.cpp
// @author octopoulos
// @version 2025-10-03

#include "stdafx.h"
#include "ui/xsettings.h"

#ifdef WITH_SDL3
#	include <SDL3/SDL_filesystem.h>
#endif // WITH_SDL3

static std::string settingsToml = "settings.ini";

const char* sAspectRatios[] = { "1:1", "4:3", "3:2", "16:9", "Native", "Window" };
const char* sEases[]        = { "None", "InOutCubic", "InOutQuad", "OutQuad" };
const char* sGames[]        = { "Custom1", "Custom2", "Custom3" };
const char* sFullScreens[]  = { "Off", "Desktop", "Screen" };
const char* sProjections[]  = { "Orthogonal", "Perspective" };
const char* sRenderModes[]  = { "None", "Screen", "Model", "Screen + Model" };
const char* sRotateModes[]  = { "Quaternion", "XYZ Euler" }; //, "Axis Angle"};
const char* sThemes[]       = { "Blender", "Classic", "Custom", "Dark", "Light", "Xemu" };
const char* sVSyncs[]       = { "Off", "On", "Adaptive" };

// global variable
XSettings xsettings;
#define XSETTINGS XSettings

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAPPING
//////////

// clang-format off
static std::vector<Config> configs = {
	// [app]
	X_STRING (XSETTINGS, app, 1, appId , ""),
	X_ENUM   (XSETTINGS, app, 1, gameId, 0, sGames),

	// [capture]
	X_STRING (XSETTINGS, capture, 0, captureDir  , "temp"),
	X_BOOL   (XSETTINGS, capture, 1, captureVideo, false),
	X_BOOL   (XSETTINGS, capture, 0, nvidiaEnc   , false),

	// [cursor]
	X_INT    (XSETTINGS, cursor, 0, cursorInit  , 500, 0, 5000),
	X_ENUM   (XSETTINGS, cursor, 0, cursorEase  , Ease_None, sEases),
	X_INT    (XSETTINGS, cursor, 0, cursorRepeat, 50 , 0, 500),
	X_FLOAT  (XSETTINGS, cursor, 0, cursorStep  , 0.5f, 0.1f, 1.0f),

	// [input]
	X_FLOAT  (XSETTINGS, input, 0, cameraSpeed, 10.0f, 1.0f, 100.0f),
	X_FLOAT  (XSETTINGS, input, 0, clickDist  , 1.0f, 0, 100.0f),
	X_INT    (XSETTINGS, input, 0, clickTime  , 300, 0, 3000),
	X_INT    (XSETTINGS, input, 0, keyInit    , 500, 0, 5000),
	X_INT    (XSETTINGS, input, 0, keyRepeat  , 50 , 0, 500),
	X_FLOAT  (XSETTINGS, input, 0, zoomKb     , 10.0f, 1.0f, 100.0f),
	X_FLOAT  (XSETTINGS, input, 0, zoomWheel  , 10.0f, 1.0f, 100.0f),

	// [map]
	X_INT    (XSETTINGS, map, 0, angleInc  , 90, 1, 180),
	X_FLOAT  (XSETTINGS, map, 0, iconSize  , 64.0f, 8.0f, 256.0f),
	X_BOOL   (XSETTINGS, map, 0, smoothPos , false),
	X_BOOL   (XSETTINGS, map, 0, smoothQuat, true),

	// [physics]
	X_FLOAT  (XSETTINGS, physics, 0, bottom     , -100.0f, -1000.0f, 0.0f),
	X_BOOL   (XSETTINGS, physics, 0, bulletDebug, false),
	X_BOOL   (XSETTINGS, physics, 0, physPaused , false),
	X_BOOL   (XSETTINGS, physics, 0, picking    , false),
	X_FLOAT  (XSETTINGS, physics, 0, rayLength  , 2000.0f, 0.0f, -1.0f),

	// [render]
	X_FLOATS (XSETTINGS, render, 0, cameraAt  , "0.0|0.0|0.0", -100.0f, 100.0f, 3),
	X_FLOATS (XSETTINGS, render, 0, cameraEye , "0.0|0.0|1.5", -100.0f, 100.0f, 3),
	X_INT    (XSETTINGS, render, 0, debug     , 0, 0, -1),
	X_FLOAT  (XSETTINGS, render, 0, distance  , 10.0f, 0.50f, 100.0f),
	X_BOOL   (XSETTINGS, render, 0, fixedView , true),
	X_FLOAT  (XSETTINGS, render, 0, fov       , 60.0f, 1.0f, 180.0f),
	X_BOOL   (XSETTINGS, render, 0, gridDraw  , true),
	X_INT    (XSETTINGS, render, 0, gridSize  , 50, 1, 500),
	X_BOOL   (XSETTINGS, render, 0, instancing, true),
	X_FLOATS (XSETTINGS, render, 0, lightDir  , "0.556|-0.508|0.185", -1.0f, 1.0f, 3),
	X_FLOAT  (XSETTINGS, render, 0, orthoZoom , 1.0f, 0.001f, 10.0f),
	X_ENUM   (XSETTINGS, render, 0, projection, Projection_Perspective, sProjections),
	X_ENUM   (XSETTINGS, render, 0, renderMode, RenderMode_Screen, sRenderModes),
	X_INT    (XSETTINGS, render, 0, reset     , 0, 0, -1),

	// [rubik]
	X_ENUM   (XSETTINGS, rubik, 0, rubikEase  , Ease_None, sEases),
	X_INT    (XSETTINGS, rubik, 0, rubikInit  , 500, 0, 5000),
	X_INT    (XSETTINGS, rubik, 0, rubikRepeat, 500, 0, 2000),

	// [system]
	X_FLOAT  (XSETTINGS, system, 0, activeMs   , 0.0f, 0.0f, 1000.0f),
	X_BOOL   (XSETTINGS, system, 0, benchmark  , false),
	X_INT    (XSETTINGS, system, 0, drawEvery  , 1, 1, 20),
	X_FLOAT  (XSETTINGS, system, 0, idleMs     , 64.0f, 0.0f, 1000.0f),
	X_FLOAT  (XSETTINGS, system, 0, idleTimeout, 2000.0f, 0.0f, 10000.0f),
	X_INT    (XSETTINGS, system, 0, ioFrameUs  , 14000, 4000, 30000),
	X_ENUM   (XSETTINGS, system, 0, vsync      , Vsync_Adaptive, sVSyncs),

	// [ui]
	X_ENUM   (XSETTINGS, ui, 0, aspectRatio, AspectRatio_Native, sAspectRatios),
	X_BOOL   (XSETTINGS, ui, 0, autoLoad   , false),
	X_BOOL   (XSETTINGS, ui, 0, autoSave   , false),
	X_FLOAT  (XSETTINGS, ui, 0, fontScale  , 0.65f, 0.3f, 10.0f),
	X_BOOL   (XSETTINGS, ui, 0, labelLeft  , true),
	X_INT    (XSETTINGS, ui, 0, objectTree , 0, 0, -1),
	X_STRINGS(XSETTINGS, ui, 0, recentFiles, "", 6),
	X_ENUM   (XSETTINGS, ui, 0, rotateMode , RotateMode_EulerXyz, sRotateModes),
	X_INT    (XSETTINGS, ui, 0, settingPad , 2, -1, 16),
	X_INT    (XSETTINGS, ui, 0, settingTree, 0, 0, -1),
	X_BOOL   (XSETTINGS, ui, 0, showVars   , false),
	X_INT    (XSETTINGS, ui, 0, testId     , 0, 0, -1),
	X_BOOL   (XSETTINGS, ui, 0, textButton , true),
	X_ENUM   (XSETTINGS, ui, 0, theme      , Theme_Blender, sThemes),
	X_FLOAT  (XSETTINGS, ui, 0, uiScale    , 1.0f, 1.0f, 2.5f),
	X_INT    (XSETTINGS, ui, 0, varTree    , 0, 0, -1),
	X_INT    (XSETTINGS, ui, 0, winOpen    , 0, 0, -1),

	// [user]
	X_STRING (XSETTINGS, user, 0, userEmail, ""),
	X_STRING (XSETTINGS, user, 1, userHost , ""),
	X_CRYPT  (XSETTINGS, user, 0, userPw   , ""),

	// [window]
	X_INT    (XSETTINGS, window, 0, dpr       , 1, 1, 4),
	X_ENUM   (XSETTINGS, window, 0, fullScreen, FullScreen_Off, sFullScreens),
	X_BOOL   (XSETTINGS, window, 0, maximized , false),
	X_INTS   (XSETTINGS, window, 0, windowPos , "-1|-1", -1, 5120, 2),
	X_INTS   (XSETTINGS, window, 0, windowSize, "1440|800", 256, 5120, 2),
};
// clang-format on

constexpr int NUM_GAMES = int(sizeof(sGames) / sizeof(*sGames));

static XSettings gameSettings[NUM_GAMES];
static bool      gameSettingsLoaded[NUM_GAMES];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

std::filesystem::path GameFolder() { return DataFolder() / GameName(); }

const char* GameName(int gameId)
{
	if (gameId == -1) gameId = xsettings.gameId;
	return (gameId >= 0 && gameId < NUM_GAMES) ? sGames[gameId] : "x";
}

void InitGameSettings()
{
	InitConfig(configs, settingsToml);
	{
		std::string exePath;
#ifdef WITH_SDL3
		exePath = SDL_GetBasePath();
		if (exePath.ends_with('/') || exePath.ends_with('\\')) exePath.pop_back();
		if (DEV_path) ui::Log("InitGameSettings: %s : %s", PathStr(std::filesystem::current_path()), Cstr(exePath));
#endif // WITH_SDL3
		InitSettings(&xsettings, sizeof(XSettings), true, exePath);
	}

	// load game settings
	for (int gameId = 0; gameId < NUM_GAMES; ++gameId)
	{
		memset(&xsettings, 0, sizeof(XSettings));
		DefaultSettings(&xsettings, sizeof(XSettings), nullptr);
		LoadGameSettings(gameId, "", "-def");
		memcpy(&gameSettings[gameId], &xsettings, sizeof(XSettings));
		gameSettingsLoaded[gameId] = true;
	}

	// update engine
	// appSettings = &xsettings;
}

void LoadGameSettings(int gameId, std::string baseName, std::string_view suffix)
{
	if (baseName.empty() && gameId >= 0 && gameId < NUM_GAMES)
		baseName = Format("%s%s.ini", sGames[gameId], Cstr(suffix));

	LoadSettings(&xsettings, sizeof(XSettings), baseName, suffix);

	// game settings
	if (gameId >= 0 && gameId < NUM_GAMES)
		xsettings.gameId = gameId;

	xsettings.windowSize[0] = (bx::max(32, xsettings.windowSize[0]) + 7) & ~15;
	xsettings.windowSize[1] = (bx::max(32, xsettings.windowSize[1]) + 3) & ~7;
}

int SaveGameSettings(std::string baseName, bool saveGame, std::string_view suffix, const USET_STR& sections)
{
	SaveSettings(baseName, suffix, sections);

	// save HD/Omega.ini as well - only app section
	if (saveGame && sections.empty())
	{
		if (const auto gameId = xsettings.gameId; gameId >= 0 && gameId < NUM_GAMES)
			SaveSettings(FormatStr("%s%s.ini", sGames[gameId], Cstr(suffix)), "", { "app", "user" });
	}
	return 1;
}
