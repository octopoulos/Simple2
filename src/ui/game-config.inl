// game-config.inl
// @author octopoulo
// @version 2025-08-02
// - add at the start of the configs mapping

#ifdef XSETTINGS

// ENGINE
/////////

// clang-format off
// [input]
X_INT   (XSETTINGS, input, 0, repeatDelay   , 500, 0, 5000),
X_INT   (XSETTINGS, input, 0, repeatInterval, 50 , 0, 500),

// [render]
X_FLOATS(XSETTINGS, render, 0, center    , "0.0|0.0|0.0", -100.0f, 100.0f, 3),
X_FLOATS(XSETTINGS, render, 0, eye       , "0.0|0.0|1.5", -100.0f, 100.0f, 3),
X_BOOL  (XSETTINGS, render, 0, fixedView , true),
X_ENUM  (XSETTINGS, render, 0, projection, Projection_Perspective, sProjections),
X_ENUM  (XSETTINGS, render, 0, renderMode, RenderMode_Screen, sRenderModes),

// [system]
X_FLOAT(XSETTINGS, system, 0, activeMs   , 0.0f, 0.0f, 1000.0f),
X_BOOL (XSETTINGS, system, 0, benchmark  , false),
X_INT  (XSETTINGS, system, 0, drawEvery  , 1, 1, 20),
X_FLOAT(XSETTINGS, system, 0, idleMs     , 64.0f, 0.0f, 1000.0f),
X_FLOAT(XSETTINGS, system, 0, idleTimeout, 2000.0f, 0.0f, 10000.0f),
X_INT  (XSETTINGS, system, 0, ioFrameUs  , 14000, 4000, 30000),
X_ENUM (XSETTINGS, system, 0, vsync      , Vsync_Adaptive, sVSyncs),

// [ui]
X_ENUM  (XSETTINGS, ui, 0, aspectRatio       , AspectRatio_Native, sAspectRatios),
X_FLOAT (XSETTINGS, ui, 0, fontScale         , 0.65f, 0.1f, 10.0f),
X_BOOL  (XSETTINGS, ui, 0, labelLeft         , true),
X_STRING(XSETTINGS, ui, 0, shortcutControls  , ""),
X_STRING(XSETTINGS, ui, 0, shortcutLog       , ""),
X_STRING(XSETTINGS, ui, 0, shortcutOpen      , ""),
X_STRING(XSETTINGS, ui, 0, shortcutPause     , ""),
X_STRING(XSETTINGS, ui, 0, shortcutScreen    , ""),
X_STRING(XSETTINGS, ui, 0, shortcutScreenshot, ""),
X_STRING(XSETTINGS, ui, 0, shortcutSettings  , ""),
X_STRING(XSETTINGS, ui, 0, shortcutStatistics, ""),
X_BOOL  (XSETTINGS, ui, 0, showStatistics    , true),
X_BOOL  (XSETTINGS, ui, 0, stretch           , true),
X_BOOL  (XSETTINGS, ui, 0, textButton        , true),
X_ENUM  (XSETTINGS, ui, 0, theme             , Theme_Dark, sThemes),
X_INT   (XSETTINGS, ui, 0, tree              , 46619401, 0, -1),
X_FLOAT (XSETTINGS, ui, 0, uiScale           , 1.0f, 1.0f, 2.5f),

// [window]
X_ENUM(XSETTINGS, window, 0, fullScreen, FullScreen_Off, sFullScreens),
X_BOOL(XSETTINGS, window, 0, maximized , false),
X_INTS(XSETTINGS, window, 0, windowPos , "-1|-1", -1, 5120, 2),
X_INTS(XSETTINGS, window, 0, windowSize, "1440|800", 256, 5120, 2),
// clang-format on

#endif // XSETTINGS
