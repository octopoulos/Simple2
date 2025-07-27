// xsettings.h
// @author octopoulos
// @version 2025-07-23

#pragma once

#include "engine-settings.h"

enum XActions_ : int
{
	Action_Off,
	Action_Pause,
	Action_Ocr,
	Action_PauseOcr,
};

enum XErrorLevels_ : int
{
	Error_None,
	Error_Ocr,
	Error_Record,
	Error_Both,
};

enum XMouseClicks_ : int
{
	Mouse_ClickOff,
	Mouse_ClickIp,
	Mouse_ClickRecaptcha,
};

enum XOcrMethods_ : int
{
	OcrMethod_Old,
	OcrMethod_New,
};

enum XPauseNames_ : int
{
	Pause_Off,
	Pause_NewName,
	Pause_NotExist,
};

enum XProcess_ : int
{
	Process_Always,
	Process_Change,
	Process_Enough,
	Process_ChangeEnough,
};

enum XTargets_ : int
{
	Target_Source,
	Target_Paint,
	Target_Ocr,
	Target_View,
	Target_Gray,
	Target_Hue,
	Target_R,
	Target_G,
	Target_B,
};

struct XSettings : public EngineSettings
{
	// [analysis]
	// 0
	str256 appId;  ///< appId to synchronize with the browser
	int    gameId; ///< 0: gori, 1: custom
	int    view;   ///< &1: analysis, &2: text, &4: division, &8: base, &16: border, &32; coord

	// [capture]
	int    actionChange;  ///< &1: pause, &2: ocr ... when frame changes
	bool   average;       ///< average frames for better IQ
	int    captureFps;    ///< 0 for no limit
	int    deviceId;      ///< capturing device Id
	int    enumDebug;     ///< show the detected windows
	str2k  enumExtract;   ///< extract windows: title|class
	bool   enumMinimize;  ///< minimize window
	int    enumPos[2];    ///< position to move the window to
	int    enumWidth[2];  ///< show windows of that exact width, <0 to disable
	int    interval[2];   ///< [start, end] of video
	bool   invertRgb;     ///< invert R and B
	int    minFrames;     ///< min # of same frames to use the scene for OCR
	int    printer;       ///< type of printer
	str512 printJob;      ///< printer job URL
	str4k  printPayload;  ///< payload for the job
	str512 printStatus;   ///< printer status URL
	int    process;       ///< when to process frames? always / at change / when collected enough frames
	float  psnrChange[2]; ///< above this value, page has changed, filter/ranking
	float  psnrSame[2];   ///< under this value, images can be combined, filter/ranking
	int    resize[2];     ///< resize capture before analysis
	bool   show;          ///< show the capture with imshow
	str512 watchDocument; ///< base folder to watch for new document pdf
	str512 watchEnvelope; ///< base folder to watch for new envelope images
	int    watchMs;       ///< timeout to capture the next file
	int    watchRetry;    ///< timeout to retry to capture next file

	// [net]
	int mousePos[2];  ///< mouse x,y to click on IP ban/recaptcha
	int netDebug;     ///< net debug level
	int netRecheck;   ///< recheck names with at least x count
	int netTransfers; ///< max parallel transfers

	// [ocr]
	int ocrMethod;  ///< 0: old=tesseract, 1: new=house
	int ocrThreads; ///< OCR threads, 0: auto
	int site;       ///< 1: SPL, 2: STR, ...

	// [ui]
	str512 recentFiles[6];   ///
	str32  shortcutDevice;   ///
	str32  shortcutOcr;      ///
	str32  shortcutOutput;   ///
	str32  shortcutPlayback; ///
	str32  shortcutStart;    ///
	str32  shortcutStop;     ///
	bool   showPlayback;     ///< show playback window
	int    target;           ///< target frame, 0: capImage, 1: paint, 2: yuv[0], 3: yuv[1], 4: yuv[2], 5: quadImage, 6: viewImage

	// [user]
	int    userEid;   ///< EID idFlags
	str256 userEmail; ///< login
	str256 userHost;  ///< ws(s) url
	str256 userPw;    ///< password
};

#include "game-settings.h"
