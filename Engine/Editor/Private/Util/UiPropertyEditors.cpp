#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"
#include "Util/EditorIconGlyphs.h"

#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <vector>

#include <imgui.h>

#include "Util/UiWidgetPrimitives.h"

namespace UiUtil
{
	bool EditFloatSliderWithInput(
	    const char* label,
	    float& value,
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		const char* sliderValueFormat = (inputFormat != nullptr && inputFormat[0] != '\0') ? "" : sliderFormat;
		bool changedBySlider = false;
		bool changedByInput = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable("##float_row", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			if (ImGui::BeginTable(
			        "##float_editor",
			        2,
			        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("slider", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthFixed, ScalarInputWidth);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat("##slider", &value, minValue, maxValue, sliderValueFormat);

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-1.0f);
				changedByInput = ImGui::InputFloat("##input", &value, 0.0f, 0.0f, inputFormat);

				ImGui::EndTable();
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changedBySlider || changedByInput;
	}

	bool EditFloat3SliderWithInput(
	    const char* label,
	    float values[3],
	    float minValue,
	    float maxValue,
	    const char* sliderFormat,
	    const char* inputFormat)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		const char* sliderValueFormat = (inputFormat != nullptr && inputFormat[0] != '\0') ? "" : sliderFormat;
		bool changedBySlider = false;
		bool changedByInput = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##float3_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			if (ImGui::BeginTable(
			        "##float3_editor",
			        1,
			        ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-1.0f);
				changedBySlider = ImGui::SliderFloat3("##slider", values, minValue, maxValue, sliderValueFormat);

				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(Float3InputWidth);
				changedByInput = ImGui::InputFloat3("##input", values, inputFormat) || changedByInput;

				ImGui::EndTable();
			}

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changedBySlider || changedByInput;
	}

	bool EditColor3(const char* label, float values[3])
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		bool changed = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##color3_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-1.0f);
			changed = ImGui::ColorEdit3(
			    "##color",
			    values,
			    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB |
			        ImGuiColorEditFlags_PickerHueBar);

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changed;
	}

	bool EditCheckbox(const char* label, bool& value)
	{
		ImFont* monoFont = SparkleUiTheme::GetMonoFont();
		bool changed = false;

		ImGui::PushID(label);
		if (ImGui::BeginTable(
		        "##checkbox_row",
		        2,
		        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
		{
			ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, PropertyLabelWidth);
			ImGui::TableSetupColumn("editor", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::AlignTextToFramePadding();
			PushFontIfAvailable(monoFont);
			ImGui::TextDisabled("%s", label);
			PopFontIfAvailable(monoFont);

			ImGui::TableSetColumnIndex(1);
			changed = ImGui::Checkbox("##checkbox", &value);

			ImGui::EndTable();
		}
		ImGui::PopID();

		return changed;
	}

}
