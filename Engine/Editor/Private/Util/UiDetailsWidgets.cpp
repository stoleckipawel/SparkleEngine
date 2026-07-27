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
	bool BeginDetailsCategory(const char* title, bool defaultOpen)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed;
		if (defaultOpen)
		{
			flags |= ImGuiTreeNodeFlags_DefaultOpen;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 4.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Header, ImGui::ColorConvertU32ToFloat4(SparkleUiPalette::SectionHeaderBackground()));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.21f, 0.22f, 0.24f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.19f, 0.26f, 0.36f, 1.0f));
		const bool open = ImGui::CollapsingHeader(title, flags);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		return open;
	}

	void EndDetailsCategory() {}

	void DrawDetailsEmptyState(const char* text)
	{
		ImGui::TextDisabled("%s", text);
	}

	void DrawDetailsValueRow(const char* label, const char* value)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(value);
		DrawDetailsEmptyUtility(2);

		EndDetailsRow();
	}

	void DrawDetailsAssetRow(const char* label, EditorIcon thumbnailIcon, const char* value, const char* typeText)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return;
		}

		ImGui::TableSetColumnIndex(1);
		const ImVec2 thumbnailSize(32.0f, 32.0f);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(
		    start,
		    ImVec2(start.x + thumbnailSize.x, start.y + thumbnailSize.y),
		    SparkleUiPalette::SceneOutlinerBadgeBackground(),
		    4.0f);
		drawList->AddRect(start, ImVec2(start.x + thumbnailSize.x, start.y + thumbnailSize.y), SparkleUiPalette::PanelHeaderBorder(), 4.0f);
		const char* thumbnailLabel = GetEditorIconGlyph(thumbnailIcon);
		const ImVec2 thumbnailTextSize = ImGui::CalcTextSize(thumbnailLabel);
		drawList->AddText(
		    ImVec2(start.x + ((thumbnailSize.x - thumbnailTextSize.x) * 0.5f), start.y + ((thumbnailSize.y - thumbnailTextSize.y) * 0.5f)),
		    SparkleUiPalette::SceneOutlinerBadgeText(),
		    thumbnailLabel);
		ImGui::Dummy(thumbnailSize);

		ImGui::SameLine(0.0f, 6.0f);
		ImGui::BeginGroup();
		const float buttonWidth = (std::max) (80.0f, ImGui::GetContentRegionAvail().x - 72.0f);
		ImGui::SetNextItemWidth(buttonWidth);
		ImGui::Button(value != nullptr ? value : "None", ImVec2(buttonWidth, 0.0f));
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Asset selector placeholder");
		}
		if (typeText != nullptr && typeText[0] != '\0')
		{
			ImGui::TextDisabled("%s", typeText);
		}
		ImGui::EndGroup();

		ImGui::SameLine(0.0f, 4.0f);
		ImGui::BeginGroup();
		ImGui::SmallButton("Use");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Use selected asset placeholder");
		}
		ImGui::SameLine(0.0f, 3.0f);
		ImGui::SmallButton("Find");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Browse asset placeholder");
		}
		ImGui::EndGroup();

		DrawDetailsEmptyUtility(2);
		EndDetailsRow();
	}

	bool EditDetailsFloat(
	    const char* label,
	    float& value,
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValue)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool changed = ImGui::DragFloat("##value", &value, speed, minValue, maxValue, format);
		if (resetValue != nullptr)
		{
			if (DrawDetailsResetButton(2, IsDifferentFromDefault(value, *resetValue)))
			{
				value = *resetValue;
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsFloat3(
	    const char* label,
	    float values[3],
	    float speed,
	    float minValue,
	    float maxValue,
	    const char* format,
	    const float* resetValues)
	{
		if (!BeginDetailsRow(label, 3))
		{
			return false;
		}

		bool changed = false;
		static constexpr const char* axisLabels[] = {"X", "Y", "Z"};
		for (int axisIndex = 0; axisIndex < 3; ++axisIndex)
		{
			ImGui::PushID(axisIndex);
			ImGui::TableSetColumnIndex(axisIndex + 1);
			if (ImGui::BeginTable(
			        "##axis_field",
			        2,
			        ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoPadOuterX | ImGuiTableFlags_NoPadInnerX))
			{
				ImGui::TableSetupColumn("axis", ImGuiTableColumnFlags_WidthFixed, DetailsAxisLabelWidth);
				ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableNextRow();

				ImGui::TableSetColumnIndex(0);
				ImGui::AlignTextToFramePadding();
				ImGui::PushStyleColor(ImGuiCol_Text, AxisColor(axisIndex));
				ImGui::TextUnformatted(axisLabels[axisIndex]);
				ImGui::PopStyleColor();

				ImGui::TableSetColumnIndex(1);
				ImGui::SetNextItemWidth(-FLT_MIN);
				changed = ImGui::DragFloat("##value", &values[axisIndex], speed, minValue, maxValue, format) || changed;

				ImGui::EndTable();
			}
			ImGui::PopID();
		}
		if (resetValues != nullptr)
		{
			if (DrawDetailsResetButton(4, IsDifferentFromDefault(values, resetValues)))
			{
				values[0] = resetValues[0];
				values[1] = resetValues[1];
				values[2] = resetValues[2];
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(4);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsColor3(const char* label, float values[3], const float* resetValues)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool changed = ImGui::ColorEdit3(
		    "##value",
		    values,
		    ImGuiColorEditFlags_Float | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_InputRGB | ImGuiColorEditFlags_PickerHueBar);
		if (resetValues != nullptr)
		{
			if (DrawDetailsResetButton(2, IsDifferentFromDefault(values, resetValues)))
			{
				values[0] = resetValues[0];
				values[1] = resetValues[1];
				values[2] = resetValues[2];
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsCheckbox(const char* label, bool& value, const bool* resetValue)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		ImGui::TableSetColumnIndex(1);
		bool changed = ImGui::Checkbox("##value", &value);
		if (resetValue != nullptr)
		{
			if (DrawDetailsResetButton(2, value != *resetValue))
			{
				value = *resetValue;
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

	bool EditDetailsText(const char* label, std::string& value, const std::string* resetValue)
	{
		if (!BeginDetailsRow(label, 1))
		{
			return false;
		}

		std::vector<char> buffer((std::max) (std::size_t{1024}, value.size() + 1u), '\0');
		std::copy(value.begin(), value.end(), buffer.begin());
		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-FLT_MIN);
		bool changed = ImGui::InputText("##value", buffer.data(), buffer.size());
		if (changed)
		{
			value = buffer.data();
		}

		if (resetValue != nullptr)
		{
			if (DrawDetailsResetButton(2, value != *resetValue))
			{
				value = *resetValue;
				changed = true;
			}
		}
		else
		{
			DrawDetailsEmptyUtility(2);
		}

		EndDetailsRow();
		return changed;
	}

}
