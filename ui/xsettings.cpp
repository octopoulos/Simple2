// xsettings.cpp
// @author octopoulos
// @version 2025-07-23

#include "stdafx.h"
#include "xsettings.h"

static std::string settingsToml = "scanner.ini";

extern const char* sAspectRatios[6];
extern const char* sFullScreens[3];
extern const char* sProjections[2];
extern const char* sRenderModes[4];
extern const char* sThemes[5];
extern const char* sVSyncs[3];

const char* sActions[]    = { "Off", "Pause", "OCR", "Pause + OCR" };
const char* sErrors[]     = { "Off", "OCR", "Record", "Both" };
const char* sGames[]      = { "Gori", "Local", "Custom" };
const char* sOcrs[]       = { "Old", "New" };
const char* sPauseNames[] = { "Off", "New Name", "Not Exist", "Both" };
const char* sPrinters[]   = { "HP 477dw"};
const char* sProcesses[]  = { "Always", "New Scene", "Enough", "New Scene + Enough" };
const char* sTargets[]    = { "Capture", "OCR", "View" };

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

	// [capture]
	X_ENUM   (XSettings, capture, 0, actionChange , Action_Ocr, sActions),
	X_BOOL   (XSettings, capture, 0, average      , true),
	X_INT    (XSettings, capture, 0, captureFps   , 0, 0, 240),
	X_INT    (XSettings, capture, 0, deviceId     , 0, 0, 8),
	X_BOOL   (XSettings, capture, 0, enumDebug    , false),
	X_STRING (XSettings, capture, 0, enumExtract  , "CZUR Scanner|MainWindow;CZUR Shine|MainWindow;ScanPreviewWindow"),
	X_BOOL   (XSettings, capture, 0, enumMinimize , false),
	X_INTS   (XSettings, capture, 0, enumPos      , "0|0", -8192, 8192, 2),
	X_INTS   (XSettings, capture, 0, enumWidth    , "0|-1", -1, 8192, 2),
	X_INTS   (XSettings, capture, 0, interval     , "0|0", -1, 240, 2),
	X_BOOL   (XSettings, capture, 0, invertRgb    , true),
	X_INT    (XSettings, capture, 0, minFrames    , 30, 0, 120),
	X_ENUM   (XSettings, capture, 0, printer      , 0, sPrinters),
	X_STRING (XSettings, capture, 0, printJob     , ""),
	X_STRING (XSettings, capture, 0, printPayload , ""),
	X_STRING (XSettings, capture, 0, printStatus  , ""),
	X_ENUM   (XSettings, capture, 0, process      , Process_ChangeEnough, sProcesses),
	X_FLOATS (XSettings, capture, 0, psnrChange   , "28.0|28.0", 0.0f, 100.0f, 2),
	X_FLOATS (XSettings, capture, 0, psnrSame     , "39.0|39.0", 0.0f, 100.0f, 2),
	X_INTS   (XSettings, capture, 0, resize       , "0|0", 0, 4096, 2),
	X_BOOL   (XSettings, capture, 0, show         , false),
	X_STRING (XSettings, capture, 0, watchDocument, ""),
	X_STRING (XSettings, capture, 0, watchEnvelope, ""),
	X_INT    (XSettings, capture, 0, watchMs      , 0, 0, 5000),
	X_INT    (XSettings, capture, 0, watchRetry   , 150, 0, 5000),

	// [net]
	X_INTS   (XSettings, net, 0, mousePos    , "0|0", 0, 5120, 2),
	X_INT    (XSettings, net, 0, netDebug    , 0, 0, 1),
	X_INT    (XSettings, net, 0, netRecheck  , 150, 0, 300),
	X_INT    (XSettings, net, 0, netTransfers, 3, 1, 16),

	// [ocr]
	X_ENUM   (XSettings, ocr, 0, ocrMethod , OcrMethod_New, sOcrs),
	X_INT    (XSettings, ocr, 0, ocrThreads, 0, 0, 16),
	X_INT    (XSettings, ocr, 0, site      , 1, 1, 16),

	// [ui]
	X_STRINGS(XSettings, ui, 0, recentFiles     , "", 6),
	X_STRING (XSettings, ui, 0, shortcutDevice  , ""),
	X_STRING (XSettings, ui, 0, shortcutOcr     , ""),
	X_STRING (XSettings, ui, 0, shortcutOutput  , ""),
	X_STRING (XSettings, ui, 0, shortcutPlayback, ""),
	X_STRING (XSettings, ui, 0, shortcutStart   , ""),
	X_STRING (XSettings, ui, 0, shortcutStop    , ""),
	X_BOOL   (XSettings, ui, 0, showPlayback    , true),
	X_ENUM   (XSettings, ui, 0, target          , Target_Source, sTargets),

	// [user]
	X_INT    (XSettings, user, 0, userEid  , 7, 1, 4095),
	X_STRING (XSettings, user, 0, userEmail, ""),
	X_STRING (XSettings, user, 1, userHost , ""),
	X_CRYPT  (XSettings, user, 0, userPw   , ""),
};
// clang-format on

#include "game-settings.inl"
