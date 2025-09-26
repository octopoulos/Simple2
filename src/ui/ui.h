// ui.h
// @author octopoulos
// @version 2025-09-21

#pragma once

#include "imgui-include.h"

class App;
using wEngine = std::weak_ptr<App>;

namespace ui
{

enum WindowTypes_
{
	WindowType_None     = 0,
	WindowType_Controls = 1 << 0,
	WindowType_Entry    = 1 << 1,
	WindowType_Log      = 1 << 2,
	WindowType_Map      = 1 << 3,
	WindowType_Object   = 1 << 4,
	WindowType_Scene    = 1 << 5,
	WindowType_Settings = 1 << 6,
	WindowType_Theme    = 1 << 7,
	WindowType_Vars     = 1 << 8,
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// COMMON
/////////

#define SHOW_TREE(flag) ((showTree & flag) ? ImGuiTreeNodeFlags_DefaultOpen : 0)

class CommonWindow
{
public:
	float       alpha   = 1.0f;            ///< window opacity
	wEngine     appWeak = {};              ///< pointer to App, to display App properties
	int         drawn   = 0;               ///< used for initial setup
	int         hidden  = 0;               ///< automatic flag, 2=cannot be hidden
	bool        isOpen  = false;           ///< manual flag, the user opened/closed the window
	std::string name    = "";              ///< Controls, Log, Scene, Settings, Theme
	ImVec2      pos     = {};              ///< current position
	ImVec2      size    = {};              ///< current size
	int         type    = WindowType_None; ///< WindowTypes_

	virtual ~CommonWindow() {}

	/// Attempt an ImGui::Begin
	/// - also get pos + size
	bool BeginDraw(int flags = 0);

	virtual void Draw() {}
};

CommonWindow& GetCommonWindow();

/// Add a combo with label left/right + special Blender support
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
bool AddCheckbox(int mode, std::string_view name, const char* labelLeft, const char* labelRight, bool* dataPtr = nullptr);

/// Add a combo with label left/right + special Blender support
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
bool AddCombo(int mode, std::string_view name, const char* label);

/// Add a combo with label left/right + special Blender support
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
bool AddCombo(int mode, std::string_view name, const char* label, const char* texts[], const VEC_INT values);

/// Add a drag float with label left/right
/// @param mode: 0: slider, &1: drag, &2: new line, &4: popup, &8: empty label, &16: no label + span all width
bool AddDragFloat(int mode, std::string_view name, const char* text, float* dataPtr = nullptr, int count = 1, float speed = 0.0005f, const char* format = "%.3f");

/// Add a drag int with label left/right
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
bool AddDragInt(int mode, std::string_view name, const char* text, int* dataPtr = nullptr, int count = 1, float speed = 1.0f, const char* format = "%d");

/// Add an input int with label left/right
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
void AddInputInt(int mode, std::string_view name, const char* label, int flags = 0, int* pint = nullptr);

/// Add an input text input with label left/right + special Blender support
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
/// @param pstring: if used, then overrides config
void AddInputText(int mode, std::string_view name, const char* label, size_t size = 256, int flags = 0, std::string* pstring = nullptr);

/// Add a menu item with a flag system
bool AddMenuFlag(std::string_view label, uint32_t& value, uint32_t flag);

/// Add a slider bool with label left/right
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
bool AddSliderBool(int mode, std::string_view name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 });

/// Create an horizontal or vertical slider, with label left/right
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
/// @param format: nullptr => slider enum, to use instead of AddCombo
bool AddSliderInt(int mode, std::string_view name, const char* text, const char* format = "%d", bool vertical = false, const ImVec2& size = { 30, 120 }, bool isBool = false);

/// Create an horizontal or vertical slider, with label left/right
/// @param mode: &4: popup, &8: empty label, &16: no label + span all width
/// @param format: nullptr => slider enum, to use instead of AddCombo
bool AddSliderInt(int mode, std::string_view name, const char* text, int* value, int count, int min, int max, const char* format = "%d");

/// Add vertical space
void AddSpace(float height = -1.0f);

/// Right click => reset to default
bool ItemEvent(std::string_view name, int index = -1);

/// Show a 2 column Stats table
void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats);

/// Utility class:
/// - indent
/// - destruction: unindent
class IndentGuard
{
private:
	float indent = 0.0f;

public:
	IndentGuard(float indent = 10.0f)
	    : indent(indent)
	{
		ImGui::Indent(indent);
	}

	~IndentGuard()
	{
		ImGui::Unindent(indent);
	}
};

/// Utility class:
/// - indent + prepare child
/// - destruction: unindent + close tree
class TreeGuard
{
private:
	float indent2  = 0.0f; ///< total indent amount, to restore later
	float paddingX = 8.0f; ///< temporary change paddingX

public:
	TreeGuard(float paddingX = 8.0f, float indent = 10.0f)
	    : paddingX(paddingX)
	{
		const auto& style = ImGui::GetStyle();
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - style.ItemSpacing.y);
		if (indent)
		{
			indent2 = indent + style.IndentSpacing;
			ImGui::Unindent(indent2);
		}
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(paddingX, 8.0f));
	}

	~TreeGuard()
	{
		ImGui::PopStyleVar();
		if (indent2) ImGui::Indent(indent2);
	}
};

#define BEGIN_CHILD(name, flag, numRow, indent)                                                                                               \
	tree |= flag;                                                                                                                             \
	ui::TreeGuard treeGuard(paddingX, indent);                                                                                                \
	if (ImGui::BeginChild(name, ImVec2(0.0f, (numRow) * ImGui::GetFrameHeightWithSpacing() + 12.0f), ImGuiChildFlags_AlwaysUseWindowPadding)) \
	{

#define BEGIN_COLLAPSE(name, flag, numRow)              \
	if (ImGui::CollapsingHeader(name, SHOW_TREE(flag))) \
	{                                                   \
		BEGIN_CHILD(name, flag, numRow, 0.0f)

#define BEGIN_PADDING()                            \
	auto&      style      = ImGui::GetStyle();     \
	const auto paddingX   = style.WindowPadding.x; \
	const int  settingPad = xsettings.settingPad;  \
	if (settingPad >= 0) ImGui::PushStyleVarX(ImGuiStyleVar_WindowPadding, xsettings.settingPad)

#define BEGIN_TREE(name, flag, numRow)                                                                                                                   \
	{                                                                                                                                                    \
		ui::IndentGuard indentGuard;                                                                                                                     \
		if (ImGui::TreeNodeEx(name, SHOW_TREE(flag) | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_Selected | ImGuiTreeNodeFlags_SpanFullWidth)) \
		{                                                                                                                                                \
			BEGIN_CHILD(name, flag, numRow, 10.0f)

#define END_COLLAPSE() \
		}              \
	}                  \
	ImGui::EndChild()

#define END_PADDING() \
	if (settingPad >= 0) ImGui::PopStyleVar()

// clang-format off
#define END_TREE()             \
				}              \
			}                  \
			ImGui::EndChild(); \
			ImGui::TreePop();  \
		}
// clang-format on

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CONTROLS
///////////

CommonWindow& GetControlsWindow();

/// Image text button aligned on a row
int DrawControlButton(uint32_t texId, const ImVec4& color, std::string name, const char* label, int textButton, float uiScale);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FX
/////

using FxFunc = void(*)(ImDrawList* drawList, ImVec2 topLeft, ImVec2, ImVec2 size, ImVec4, float time);

std::vector<std::pair<std::string, FxFunc>> GetFxFunctions();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// LOG
//////

CommonWindow& GetLogWindow();

/// Add Log - std::string version
/// @param id: 0: log, 1: error, 2: info, 3: warning
void AddLog(int id, const std::string& text);

/// Clear part_of/all the log
void ClearLog(int id);

/// Erase the previous x lines
void RemovePrevious(std::string_view prevText);

/// Indent/unindent log
void Tab(int value);

#undef Log
#undef LogError
#undef LogInfo
#undef LogOutput
#undef LogWarning

#define Log(...)        AddLog(0, fmt::format(__VA_ARGS__))
#define LogError(...)   AddLog(1, fmt::format(__VA_ARGS__))
#define LogInfo(...)    AddLog(2, fmt::format(__VA_ARGS__))
#define LogOutput(...)  AddLog(4, fmt::format(__VA_ARGS__))
#define LogWarning(...) AddLog(3, fmt::format(__VA_ARGS__))

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAP
//////

CommonWindow& GetMapWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MENU
///////

/// Current menu height
float GetMenuHeight();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// OBJECT
/////////

CommonWindow& GetObjectWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SCENE
////////

CommonWindow& GetSceneWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SETTINGS
///////////

CommonWindow& GetSettingsWindow();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THEME
////////

CommonWindow& GetThemeWindow();
ImFont*       FindFont(std::string_view name);
void          UpdateFonts();
void          UpdateTheme();

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// UI
/////

/// Add a menu item for a window toggle
bool AddMenu(const char* text, const char* shortcut, CommonWindow& window);

/// Draw all UI
void DrawWindows();

/// Create a list of the windows
void ListWindows(std::shared_ptr<App> &app);

/// Save windows open states
void SaveWindows();

/// Set alpha for the next window
bool SetAlpha(float alpha);

/// Hide/unhide windows
///  - only change hidden, not isOpen
bool ShowWindows(bool show, bool force);

} // namespace ui
