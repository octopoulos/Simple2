// ui-common.cpp
// @author octopoulos
// @version 2025-08-04

#include "stdafx.h"
#include "ui/ui.h"
//
#include "ui/xsettings.h"

namespace ui
{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// HELPERS
//////////

static void LabelLeft(const char* label, int components)
{
	float valueWidth = 0.0f;
	if (!xsettings.labelLeft)
		valueWidth = ImGui::CalcItemWidth();
	else
	{
		ImGui::AlignTextToFramePadding();

		const float spacingX = 6.0f; //ImGui::GetStyle().ItemInnerSpacing.x;
		const float width    = ImGui::GetContentRegionAvail().x;
		valueWidth           = width * 0.6f - spacingX;

		if (auto labelEnd = ImGui::FindRenderedTextEnd(label); label != labelEnd)
		{
			const float posX = ImGui::GetCursorPosX();
			const auto  size = ImGui::CalcTextSize(label);

			ImGui::SetCursorPosX(posX + width * 0.4f - size.x - spacingX);
			ImGui::TextEx(label, labelEnd);
			ImGui::SameLine(0.0f, spacingX * 2);
		}
	}

	ImGui::PushMultiItemsWidths(components, valueWidth);
}

static void LabelRight(const char* label)
{
	if (!xsettings.labelLeft)
	{
		const float spacingX = 6.0f; //ImGui::GetStyle().ItemInnerSpacing.x;
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
			ImGui::PushStyleColor(ImGuiCol_FrameBg       , ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.19f, 0.19f, 0.19f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive , ImVec4(0.27f, 0.38f, 0.56f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_Button        , ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered , ImVec4(0.19f, 0.19f, 0.19f, 1.00f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive  , ImVec4(0.27f, 0.38f, 0.56f, 1.00f));
			return 6;
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg       , ImVec4(0.11f, 0.11f, 0.11f, 0.88f)); // 29
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.14f, 1.00f)); // 35
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive , ImVec4(0.09f, 0.09f, 0.09f, 1.00f)); // 24
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS
////////////

#define LABEL_ID(label) fmt::format("##{}", label).c_str()

bool AddCombo(const std::string& name, const char* label)
{
	bool result = false;
	if (auto config = ConfigFind(name))
	{
		LabelLeft(label, 1);
		const int pushed = PushBlender(1);

		result = ImGui::Combo(LABEL_ID(label), (int*)config->ptr, config->names, config->count);
		result |= ItemEvent(name);

		PopBlender(pushed);
		LabelRight(label);
	}
	return result;
}

bool AddCombo(const std::string& name, const char* label, const char* texts[], const VEC_INT values)
{
	bool result = false;
	if (auto config = ConfigFind(name))
	{
		auto it    = std::find(values.begin(), values.end(), *(int*)config->ptr);
		int  index = (it != values.end()) ? TO_INT(std::distance(values.begin(), it)) : 0;

		LabelLeft(label, 1);
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
/// @param mode: 0:slider, 1:drag
static bool AddDragScalarN(int mode, const std::string& name, const char* label, ImGuiDataType data_type, size_t type_size, void* p_data, int components, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags = 0)
{
	const float spacingX     = ImGui::GetStyle().ItemInnerSpacing.x;
	bool        valueChanged = false;

	ImGui::BeginGroup();
	ImGui::PushID(label);

	components = std::max(components, 1);
	LabelLeft(label, components);

	for (int i = 0; i < components; ++i)
	{
		ImGui::PushID(i);
		if (i > 0)
			ImGui::SameLine(0, spacingX);

		if (mode == 0)
			valueChanged |= ImGui::SliderScalar(fmt::format("##{}{}", name, i).c_str(), data_type, p_data, p_min, p_max, format, flags);
		else
			valueChanged |= ImGui::DragScalar(fmt::format("##{}{}", name, i).c_str(), data_type, p_data, v_speed, p_min, p_max, format, flags);

		valueChanged |= ItemEvent(name, i);

		ImGui::SameLine();
		ImGui::PopID();
		ImGui::PopItemWidth();
		p_data = (void*)((char*)p_data + type_size);
	}
	ImGui::PopID();

	LabelRight(label);

	ImGui::EndGroup();
	return valueChanged;
}

bool AddDragFloat(const std::string& name, const char* text, float speed, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(1, name, text, ImGuiDataType_Float, sizeof(float), (float*)config->ptr, config->count, speed, &config->minFloat, &config->maxFloat, format);
	return result;
}

bool AddDragInt(const std::string& name, const char* text, float speed, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(1, name, text, ImGuiDataType_S32, sizeof(int32_t), (int*)config->ptr, config->count, speed, &config->minInt, &config->maxFloat, format);
	return result;
}

void AddInputText(const std::string& name, const char* label, size_t size, int flags)
{
	LabelLeft(label, 1);
	const int pushed = PushBlender(2);

	if (auto config = ConfigFind(name))
		ImGui::InputText(LABEL_ID(label), (char*)config->ptr, size, flags);

	PopBlender(pushed);
	LabelRight(label);
}

bool AddSliderBool(const std::string& name, const char* text, const char* format, bool vertical, const ImVec2& size)
{
	return AddSliderInt(name, text, format, vertical, size, true);
}

bool AddSliderFloat(const std::string& name, const char* text, const char* format)
{
	bool result = false;
	if (auto config = ConfigFind(name))
		result = AddDragScalarN(0, name, text, ImGuiDataType_Float, sizeof(float), (float*)config->ptr, config->count, 1.0f, &config->minFloat, &config->maxFloat, format);
	return result;
}

bool AddSliderInt(const std::string& name, const char* text, const char* format, bool vertical, const ImVec2& size, bool isBool)
{
	bool result = false;
	if (auto config = ConfigFind(name))
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

bool AddSliderInt(const std::string& name, const char* text, int* value, int count, int min, int max, const char* format)
{
	return AddDragScalarN(0, name, text, ImGuiDataType_S32, sizeof(int32_t), value, count, 1.0f, &min, &max, format);
}

void AddSpace(float height)
{
	if (height < 0) height = -height * ImGui::GetStyle().WindowPadding.y;
	ImGui::Dummy(ImVec2(0, height * xsettings.uiScale));
}

bool ItemEvent(const std::string& name, int index)
{
	if (auto config = ConfigFind(name))
	{
		if (ImGui::IsItemClicked(1))
		{
			config->ResetDefault(index);
			return true;
		}
	}
	return false;
}

void ShowTable(const std::vector<std::tuple<std::string, std::string>>& stats)
{
	if (ImGui::BeginTable("Stats", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchSame))
	{
		for (const auto& [name, value] : stats)
		{
			ImGui::TableNextRow();
			ImGui::TableSetColumnIndex(0);
			ImGui::TextUnformatted(name.c_str());
			ImGui::TableSetColumnIndex(1);
			ImGui::TextUnformatted(value.c_str());
		}
		ImGui::EndTable();
	}
}

static CommonWindow commonWindow;

CommonWindow& GetCommonWindow() { return commonWindow; }

} // namespace ui
