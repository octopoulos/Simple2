// xsettings.h
// @author octopoulos
// @version 2025-09-17

#pragma once

/// Aspect ratio enumeration
enum XAspectRatios_ : int
{
	AspectRatio_1_1,    ///< 1
	AspectRatio_4_3,    ///< 1.3333
	AspectRatio_3_2,    ///< 1.5
	AspectRatio_16_9,   ///< 1.777
	AspectRatio_Native, ///
	AspectRatio_Window, ///
};

enum XChanges_ : int
{
	Change_Analysis  = 1,
	Change_Derivator = 2,
	Change_Changed   = 128,
};

enum XEases_ : int
{
	Ease_None       = 0,
	Ease_InOutCubic = 1,
	Ease_InOutQuad  = 2,
	Ease_OutQuad    = 3,
};

enum XProjections_ : int
{
	Projection_Orthographic,
	Projection_Perspective,
};

enum XRenderModes_ : int
{
	RenderMode_None,
	RenderMode_Screen,
	RenderMode_Model,
	RenderMode_Both,
};

enum XRotateModes_ : int
{
	RotateMode_Quaternion,
	RotateMode_EulerXyz,
	// RotateMode_AxisAngle,
};

enum XThemes_ : int
{
	Theme_Blender,
	Theme_Classic,
	Theme_Custom,
	Theme_Dark,
	Theme_Light,
	Theme_Xemu,
};

enum XVSyncs_ : int
{
	Vsync_Off,
	Vsync_On,
	Vsync_Adaptive,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAPPING
//////////

struct XSettings
{
	// [app]
	str256 appId;  ///< appId to synchronize with the browser
	int    gameId; ///< 0: gori, 1: custom

	// [capture]
	str2k captureDir;   ///< folder for image + video captures
	bool  captureVideo; ///< allow video capture
	bool  nvidiaEnc;    ///< use nVidia encoding

	// [input]
	float cameraSpeed;  ///< camera movement speed
	int   cursorEase;   ///< easing function
	int   cursorInit;   ///< Ms wait for cursor repeat to kick in
	int   cursorRepeat; ///< Ms cursor repeat interval
	int   keyInit;      ///< Ms wait for key repeat to kick in
	int   keyRepeat;    ///< Ms key repeat interval
	float zoomKb;       ///< zoom speed with the keyboard
	float zoomWheel;    ///< zoom speed with the mouse wheel

	// [map]
	int   angleInc;   ///< angle increment in degrees
	float iconSize;   ///< map icon size
	bool  smoothPos;  ///< smooth translation
	bool  smoothQuat; ///< smooth rotation

	// [physics]
	float bottom;      ///< remove object if drops below this
	bool  bulletDebug; ///< bullet debug draw
	bool  physPaused;  ///< paused physics
	bool  picking;     ///< raycast picking
	float rayLength;   ///< ray max distance

	// [render]
	float    cameraAt[3];  ///< camera lookAt
	float    cameraEye[3]; ///< camera eye
	uint32_t debug;        ///< bgfx: debug
	float    distance;     ///< distance between camera and cursor
	bool     fixedView;    ///< don't move the view with the mouse
	float    fov;          ///< field of view
	bool     gridDraw;     ///< draw grid?
	int      gridSize;     ///< number of grid divisions
	bool     instancing;   ///< use mesh instancing
	float    lightDir[3];  ///< light direction
	float    orthoZoom;    ///< zoom in orthographic projection
	int      projection;   ///< 0: ortho, 1: perspective
	int      renderMode;   ///< &1: screen, &2: model
	uint32_t reset;        ///< bgfx: reset

	// [rubik]
	int rubikEase;   ///< easing function
	int rubikInit;   ///< Ms wait for rubik repeat to kick in
	int rubikRepeat; ///< Ms rubik repeat interval

	// [system]
	float activeMs;    ///< ms per frame when mouse moving + focused
	bool  benchmark;   ///< analyze every frame for benchmark purpose
	int   drawEvery;   ///< render every frame?
	float idleMs;      ///< ms per frame when idle or not focused
	float idleTimeout; ///< after how many ms is it considered idle?
	int   ioFrameUs;   ///< how many us to spend in I/O per frame
	int   vsync;       ///< off, on, adaptive

	// [ui]
	int   aspectRatio;    ///< frame aspect ratio
	bool  autoLoad;       ///< load most recent scene at startup
	bool  autoSave;       ///< save scene at every change
	float fontScale;      ///< FontScaleMain
	bool  labelLeft;      ///< labels on the left of the inputs
	int   objectTree;     ///< which setting tree nodes are open in the Object panel
	str2k recentFiles[6]; ///< recent files for quick load
	int   rotateMode;     ///< quaternion, xyz-Euler, axis angle
	int   settingPad;     ///< override windows padding for CommonWindow
	int   settingTree;    ///< which setting tree nodes are open in the Settings panel
	bool  showVars;       ///< show variables (VarsUi)
	int   testId;         ///< TestUI FX function
	int   textButton;     ///< show text under the button
	int   theme;          ///< Blender, Dark, Light
	float uiScale;        ///< general scale
	int   varTree;        ///< which var tree nodes are open in the panel
	int   winOpen;        ///< which windows are open (flag)

	// [user]
	str256 userEmail; ///< login
	str256 userHost;  ///< ws(s) url
	str256 userPw;    ///< password

	// [window]
	int  dpr;           ///< device pixel ratio, usually 1 but on OSX: 2
	int  fullScreen;    ///< 1: FS, 2: FS desktop
	bool maximized;     ///< start in maximized window
	int  windowPos[2];  ///< -1 = centered
	int  windowSize[2]; ///< [width, height]
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

/// Populate the settingsMap + find the settings folder
void InitGameSettings();

/// Load settings.ini
void LoadGameSettings(int gameId = -1, std::string baseName = "", std::string_view suffix = "");

/// Save settings.ini
/// @param sections: empty to save all
int SaveGameSettings(std::string baseName = "", bool saveGame = true, std::string_view suffix = "", const USET_STR& sections = {});

/// Get the settings folder (C++)
std::filesystem::path GameFolder();

/// Get the game string
/// @param gameId: -1: get current gameId
/// @returns "x" if incorrect gameId
const char* GameName(int gameId = -1);

extern XSettings xsettings;
