// EngineSettings.h
// @author octopoulos
// @version 2025-08-02

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

struct EngineSettings
{
	// [input]
	int64_t repeatDelay;    ///< Ms wait for repeat to kick in
	int64_t repeatInterval; ///< Ms repeat interval

	// [render]
	float center[3];  ///< scene center
	float eye[3];     ///< scene eye
	bool  fixedView;  ///< don't move the view with the mouse
	int   projection; ///< 0: ortho, 1: perspective
	int   renderMode; ///< &1: screen, &2: model

	// [system]
	float activeMs;    ///< ms per frame when mouse moving + focused
	bool  benchmark;   ///< analyze every frame for benchmark purpose
	int   drawEvery;   ///< render every frame?
	float idleMs;      ///< ms per frame when idle or not focused
	float idleTimeout; ///< after how many ms is it considered idle?
	int   ioFrameUs;   ///< how many us to spend in I/O per frame
	int   vsync;       ///< off, on, adaptive

	// [ui]
	int   aspectRatio;        ///< frame aspect ratio
	float fontScale;          ///< FontScaleMain
	str32 shortcutControls;   ///
	str32 shortcutLog;        ///
	str32 shortcutOpen;       ///
	str32 shortcutPause;      ///
	str32 shortcutScreen;     ///
	str32 shortcutScreenshot; ///
	str32 shortcutSettings;   ///
	str32 shortcutStatistics; ///
	bool  showStatistics;     ///< show statistics window
	int   stretch;            ///< stretch capture to window
	int   textButton;         ///< show text under the button
	int   theme;              ///
	int   tree;               ///< which tree nodes are open in the panel
	float uiScale;            ///

	// [window]
	int  fullScreen;    ///< 1: FS, 2: FS desktop
	bool maximized;     ///< start in maximized window
	int  windowPos[2];  ///< -1 = centered
	int  windowSize[2]; ///< [width, height]
};

/// Populate the settingsMap + find the settings folder
void InitEngineSettings();

/// Load settings.ini
void LoadEngineSettings(std::string baseName = "", std::string_view suffix = "");

/// Save settings.ini
/// @param sections: empty to save all
int SaveEngineSettings(std::string baseName = "", std::string_view suffix = "", const USET_STR& sections = {});

extern EngineSettings* appSettings;
