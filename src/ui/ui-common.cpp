// ui-common.cpp
// @author octopoulos
// @version 2025-09-18

#include "stdafx.h"
#include "ui/ui.h"
//
#include "ui/xsettings.h" // xsettings

namespace ui
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

/// Label on the left
/// @param mode: &4: popup mode, &8: empty label but aligned, &16: skip
static void LabelLeft(int mode, const char* label, int components, float spaceRight = 0.0f, float widthRatio = 0.4f, float forceLeft = -1.0f)
{
	if (mode & 16)
	{
		forceLeft  = 0.0f;
		label      = "";
		widthRatio = 0.0f;
	}
	else
	{
		if (mode & 8) label = "";
		if (mode & 4) widthRatio = 0.1f;
	}
	if ((mode & 32) && spaceRight <= 0.0f) spaceRight = 76.0f;

	float valueWidth = 0.0f;
	if (!xsettings.labelLeft && forceLeft < 0.0f)
		valueWidth = ImGui::CalcItemWidth();
	else
	{
		ImGui::AlignTextToFramePadding();

		const float spacingX = (forceLeft >= 0.0f) ? forceLeft : 6.0f;
		const float width    = ImGui::GetContentRegionAvail().x;
		valueWidth           = width * (1.0f - widthRatio) - spacingX - spaceRight;

		if (auto labelEnd = ImGui::FindRenderedTextEnd(label))
		{
			const float posX = ImGui::GetCursorPosX();
			const auto  size = ImGui::CalcTextSize(label);

			ImGui::SetCursorPosX(posX + width * widthRatio - size.x - spacingX);
			ImGui::TextEx(label, labelEnd);
			ImGui::SameLine(0.0f, spacingX * 2);
		}
	}

	ImGui::PushMultiItemsWidths(components, valueWidth);
}

static void LabelRight(const char* label, float forceRight = -1.0f)
{
	if (!xsettings.labelLeft || forceRight >= 0.0f)
	{
		const float spacingX = (forceRight >= 0.0f) ? forceRight : 6.0f; //ImGui::GetStyle().ItemInnerSpacing.x;
		if (auto label_end = ImGui::FindRenderedTextEnd(label); label != label_end)
		{
			ImGui::SameLine(0, spacingX);
			ImGui::TextEx(label, label_end);
		}
	}
}

/// mode: 1=combo 2=text
static int PushBlender(int mode)
{
	if (xsettings.theme == Theme_Blender)
	{
		// clang-format off
		if (mode == 1)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg       , ImVec4(0.160f, 0.160f, 0.160f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.190f, 0.190f, 0.190f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive , ImVec4(0.270f, 0.380f, 0.560f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_Button        , ImVec4(0.160f, 0.160f, 0.160f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered , ImVec4(0.190f, 0.190f, 0.190f, 1.000f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive  , ImVec4(0.270f, 0.380f, 0.560f, 1.000f));
			return 6;
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg       , ImVec4(0.110f, 0.110f, 0.110f, 0.880f)); // 29
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.140f, 0.140f, 0.140f, 1.000f)); // 35
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive , ImVec4(0.090f, 0.090f, 0.090f, 1.000f)); // 24
			return 3;
		}
		// clang-format on
	}

	return 0;
}

static void PopBlender(int pushed)
{
	if (pushed) ImGui::PopStyleColor(pushed);
}

static int InputTextCallback(ImGuiInputTextCallbackData* data)
{
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		auto str = (std::string*)data->UserData;
		str->resize(data->BufTextLen);
		data->Buf = str->data();
	}
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

#define LABEL_ID(label) Format("##%s", label)

bool AddCheckbox(int mode, std::string_view name, const char* labelLeft, const char* labelRight, bool* dataPtr)
{
	if (!dataPtr)
	{
		if (auto config = ConfigFind(name, "AddCheckBox"))
			dataPtr = (bool*)config->ptr;
		else
			return false;
	}

	LabelLeft(mode, labelLeft, 1);

	bool result = ImGui::Checkbox(LABEL_ID(Cstr(name)), dataPtr);
	result |= ItemEvent(name);

	LabelRight(labelRight, 8.0f);
	return result;
}

bool AddCombo(int mode, std::string_view name, const char* label)
{
	bool result = false;
	if (auto config = ConfigFind(name, "AddCombo"))
	{
		LabelLeft(mode, label, 1);
		const int pushed = PushBlender(1);

		result = ImGui::Combo(LABEL_ID(label), (int*)config->ptr, config->names, config->count);
		result |= ItemEvent(name);

		PopBlender(pushed);
		LabelRight(label);
	}
	return result;
}

bool AddCombo(int mode, std::string_view name, const char* label, const char* texts[], const VEC_INT values)
{
	bool result = false;
	if (auto config = ConfigFind(name, "AddCombo"))
	{
		auto it    = std::find(values.begin(), values.end(), *(int*)config->ptr);
		int  index = (it != values.end()) ? TO_INT(std::distance(values.begin(), it)) : 0;

		LabelLeft(mode, label, 1);
		const int pushed = PushBlender(1);

		if (ImGui::Combo(LABEL_ID(label), &index, texts, TO_INT(values.size())))
		{
			*(int*)config->ptr = values[index];
			result             = true;
		}
		result |= ItemEvent(name);

		PopBlender(pushed);
		LabelRight(label);
	}
	return result;
}

/// Same as DraggScalarN but can be reset with right click
/// @param mode: 0: slider, &1: drag, &2: vertical (new line), &4: popup (label above)
static bool AddDragScalarN(int mode, std::string_view name, const char* label, ImGuiDataType data_type, size_t type_size, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags = 0)
{
	const bool  isHori       = !(mode & 2);
	const bool  isPopup      = (mode & 4);
	const float spacingX     = ImGui::GetStyle().ItemInnerSpacing.x;
	const char* subLabels[]  = { "X", "Y", "Z", "W" };
	bool        valueChanged = false;

	ImGui::BeginGroup();
	ImGui::PushID(label);

	components = bx::max(components, 1);
	if (isPopup)
	{
		AddSpace(-0.1f);
		ImGui::Text("%s:", label);
	}
	else if (isHori)
		LabelLeft(mode, label, components);

	for (int i = 0; i < components; ++i)
	{
		const char* subLabel = (components > 1) ? subLabels[i] : "";
		if (isPopup)
			LabelLeft(mode, subLabel, 1, 0.1f);
		else if (!isHori)
			LabelLeft(mode, Format("%s%s%s", !i ? label : "", !i ? " " : "", subLabel), 1);

		ImGui::PushID(i);
		if (isHori && i > 0)
			ImGui::SameLine(0, spacingX);

		if (!(mode & 1))
			valueChanged |= ImGui::SliderScalar(Format("##%s%d", Cstr(name), i), data_type, p_data, p_min, p_max, format, flags);
		else
			valueChanged |= ImGui::DragScalar(Format("##%s%d", Cstr(name), i), data_type, p_data, v_speed, p_min, p_max, format, flags);

		valueChanged |= ItemEvent(name, i);

		if (isHori) ImGui::SameLine();
		ImGui::PopID();
		ImGui::PopItemWidth();
		p_data = (void*)((char*)p_data + type_size);

		if (!isHori) LabelRight(label);
	}
	ImGui::PopID();

	if (isHori) LabelRight(label);

	ImGui::EndGroup();
	return valueChanged;
}

bool AddDragFloat(int mode, std::string_view name, const char* text, float* dataPtr, int count, float speed, const char* format)
{
	bool result = false;
	if (dataPtr)
		result = AddDragScalarN(mode, name, text, ImGuiDataType_Float, sizeof(float), dataPtr, count, speed, nullptr, nullptr, format);
	else if (auto config = ConfigFind(name, "AddDragFloat"))
		result = AddDragScalarN(mode, name, text, ImGuiDataType_Float, sizeof(float), (float*)config->ptr, config->count, speed, &config->minFloat, &config->maxFloat, format);
	return result;
}

bool AddDragInt(int mode, std::string_view name, const char* text, int* dataPtr, int count, float speed, const char* format)
{
	bool result = false;
	if (dataPtr)
		result = AddDragScalarN(mode, name, text, ImGuiDataType_S32, sizeof(int), dataPtr, count, speed, nullptr, nullptr, format);
	else if (auto config = ConfigFind(name, "AddDragInt"))
		result = AddDragScalarN(mode, name, text, ImGuiDataType_S32, sizeof(int32_t), (int*)config->ptr, config->count, speed, &config->minInt, &config->maxFloat, format);
	return result;
}

void AddInputText(int mode, std::string_view name, const char* label, size_t size, int flags, std::string* pstring)
{
	LabelLeft(mode, label, 1);
	const int pushed = PushBlender(2);

	if (pstring)
	{
		flags |= ImGuiInputTextFlags_CallbackResize;
		ImGui::InputText(LABEL_ID(label), pstring->data(), pstring->capacity() + 1, flags, InputTextCallback, pstring);
	}
	else if (auto config = ConfigFind(name, "AddInputText"))
		ImGui::InputText(LABEL_ID(label), static_cast<char*>(config->ptr), size, flags);

	PopBlender(pushed);
	LabelRight(label);
}

bool AddMenuFlag(std::string_view label, uint32_t& value, uint32_t flag)
{
	bool selected = value & flag;
	if (ImGui::MenuItem(Cstr(label), nullptr, &selected))
	{
		if (selected)
			value |= flag;
		else
			value &= ~flag;
		return true;
	}
	return false;
}

bool AddSliderBool(int mode, std::string_view name, const char* text, const char* format, bool vertical, const ImVec2& size)
{
	return AddSliderInt(mode, name, text, format, vertical, size, true);
}

bool AddSliderInt(int mode, std::string_view name, const char* text, const char* format, bool vertical, const ImVec2& size, bool isBool)
{
	bool result = false;
	if (auto config = ConfigFind(name, "AddSliderInt"))
	{
		// slider enum?
		const int count   = config->type == 'e' ? 1 : config->count;
		const int value   = *(int*)config->ptr;
		auto      format2 = (isBool ? ((value == 0) ? "Off" : "On") : (format ? format : ((value >= 0 && value < config->count) ? config->names[value] : "%d")));
		const int min     = format ? config->minInt : 0;
		const int max     = format ? config->maxInt : config->count - 1;

		if (count == 1 && vertical)
		{
			result = ImGui::VSliderInt(text, size, (int*)config->ptr, min, max, format2);
			result |= ItemEvent(name);
		}
		else result = AddDragScalarN(0, name, text, ImGuiDataType_S32, sizeof(int32_t), (int*)config->ptr, count, 1.0f, &min, &max, format2);
	}
	return result;
}

bool AddSliderInt(int mode, std::string_view name, const char* text, int* value, int count, int min, int max, const char* format)
{
	return AddDragScalarN(mode, name, text, ImGuiDataType_S32, sizeof(int32_t), value, count, 1.0f, &min, &max, format);
}

void AddSpace(float height)
{
	if (height < 0) height = -height * ImGui::GetStyle().WindowPadding.y;
	ImGui::Dummy(ImVec2(0.0f, height * xsettings.uiScale));
}

bool ItemEvent(std::string_view name, int index)
{
	if (name.size() && name.front() != '.')
	{
		if (auto config = ConfigFind(name, "ItemEvent"))
		{
			if (ImGui::IsItemClicked(1))
			{
				config->ResetDefault(index);
				return true;
			}
		}
	}
	return false;
}

void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats)
{
	if (ImGui::BeginTable("2ways", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg))
	{
		ImGui::TableSetupColumn("##Name" , ImGuiTableColumnFlags_WidthStretch, 3.0f);
		ImGui::TableSetupColumn("##Value", ImGuiTableColumnFlags_WidthStretch, 5.0f);

		for (const auto& [name, value] : stats)
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(value.c_str());
		}
		ImGui::EndTable();
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CommonWindow
///////////////

bool CommonWindow::BeginDraw(int flags)
{
	if (!isOpen || (hidden & 1)) return false;

	if (!ImGui::Begin(name.c_str(), &isOpen, flags))
	{
		ImGui::End();
		return false;
	}
	pos  = ImGui::GetWindowPos();
	size = ImGui::GetWindowSize();
	return true;
}

static CommonWindow commonWindow;

CommonWindow& GetCommonWindow() { return commonWindow; }

} // namespace ui
