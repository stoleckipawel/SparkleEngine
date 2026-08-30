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
	const float PropertyLabelWidth = 78.0f;
	const float ScalarInputWidth = 86.0f;
	const float Float3InputWidth = 188.0f;
	const float DetailsLabelWidth = 132.0f;
	const float DetailsAxisLabelWidth = 12.0f;
	const float DetailsRowVerticalPadding = 1.0f;
	const float PlaceholderIconSize = 14.0f;
	const float DetailsUtilityColumnWidth = 24.0f;
	const float DetailsResetButtonSize = 14.0f;
	const float DetailsDirtyEpsilon = 0.0001f;

	ImVec4 DetailsGridLineColor() noexcept
	{
		ImVec4 color = ImGui::ColorConvertU32ToFloat4(SparkleUiPalette::PanelHeaderBorder());
		color.w = 0.18f;
		return color;
	}

	ImU32 DetailsRowBackgroundColor() noexcept
	{
		return IM_COL32(24, 25, 28, 150);
	}

	void PushFontIfAvailable(ImFont* font)
	{
		if (font != nullptr)
		{
			ImGui::PushFont(font);
		}
	}

	void PopFontIfAvailable(ImFont* font)
	{
		if (font != nullptr)
		{
			ImGui::PopFont();
		}
	}

	void DrawHeaderBar(
	    const char* title,
	    const char* trailingText,
	    float height,
	    ImU32 backgroundColor,
	    ImU32 borderColor,
	    ImFont* titleFont,
	    ImFont* trailingFont,
	    const ImVec2& padding)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 start = ImGui::GetCursorScreenPos();
		const float width = ImGui::GetContentRegionAvail().x;
		const ImVec2 end = ImVec2(start.x + width, start.y + height);

		drawList->AddRectFilled(start, end, backgroundColor);
		drawList->AddLine(ImVec2(start.x, end.y - 1.0f), ImVec2(end.x, end.y - 1.0f), borderColor, 1.0f);

		ImGui::InvisibleButton("##header_bar", ImVec2(width, height));
		ImGui::SetCursorScreenPos(ImVec2(start.x + padding.x, start.y + padding.y));
		PushFontIfAvailable(titleFont);
		ImGui::TextUnformatted(title);
		PopFontIfAvailable(titleFont);

		if (trailingText != nullptr && trailingText[0] != '\0')
		{
			const float trailingWidth = ImGui::CalcTextSize(trailingText).x;
			const float trailingRightPadding = padding.x + ImGui::GetStyle().ScrollbarSize;
			ImGui::SetCursorScreenPos(ImVec2(end.x - trailingRightPadding - trailingWidth, start.y + padding.y));
			PushFontIfAvailable(trailingFont);
			ImGui::TextDisabled("%s", trailingText);
			PopFontIfAvailable(trailingFont);
		}

		ImGui::SetCursorScreenPos(ImVec2(start.x, end.y + 4.0f));
	}

	void DrawRightAlignedText(const char* value)
	{
		const float valueWidth = ImGui::CalcTextSize(value).x;
		const float currentX = ImGui::GetCursorPosX();
		const float offset = (std::max) (0.0f, ImGui::GetContentRegionAvail().x - valueWidth);
		ImGui::SetCursorPosX(currentX + offset);
		ImGui::TextUnformatted(value);
	}

	ImU32 AxisColor(int axisIndex) noexcept
	{
		switch (axisIndex)
		{
			case 0:
				return IM_COL32(205, 82, 82, 210);
			case 1:
				return IM_COL32(82, 178, 104, 210);
			case 2:
				return IM_COL32(82, 122, 208, 210);
			default:
				return ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::TextMuted());
		}
	}

	bool IsDifferentFromDefault(float value, float defaultValue) noexcept
	{
		return std::fabs(value - defaultValue) > DetailsDirtyEpsilon;
	}

	bool IsDifferentFromDefault(const float values[3], const float defaultValues[3]) noexcept
	{
		return IsDifferentFromDefault(values[0], defaultValues[0]) || IsDifferentFromDefault(values[1], defaultValues[1])
		    || IsDifferentFromDefault(values[2], defaultValues[2]);
	}

	ImVec4 WithAlpha(ImVec4 color, float alpha) noexcept
	{
		color.w *= alpha;
		return color;
	}

	void DrawCenteredGlyph(ImDrawList* drawList, const ImVec2& start, const ImVec2& size, const char* glyph, ImU32 color) noexcept
	{
		const ImVec2 textSize = ImGui::CalcTextSize(glyph);
		const ImVec2 textPosition(start.x + ((size.x - textSize.x) * 0.5f), start.y + ((size.y - textSize.y) * 0.5f));
		drawList->AddText(textPosition, color, glyph);
	}

	bool BeginDetailsRow(const char* label, int valueColumnCount)
	{
		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8.0f, DetailsRowVerticalPadding));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 2.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		const ImVec4 gridLineColor = DetailsGridLineColor();
		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, gridLineColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, gridLineColor);
		ImGui::PushStyleColor(ImGuiCol_Separator, gridLineColor);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.19f, 0.21f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.22f, 0.23f, 0.25f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.18f, 0.29f, 0.45f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_CheckMark, SparkleUiPalette::AccentStrong());

		ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingStretchSame;
		tableFlags |= ImGuiTableFlags_NoPadOuterX;
		tableFlags |= ImGuiTableFlags_BordersInnerV;

		if (!ImGui::BeginTable("##details_row", valueColumnCount + 2, tableFlags))
		{
			ImGui::PopStyleColor(7);
			ImGui::PopStyleVar(5);
			ImGui::PopID();
			return false;
		}

		ImGui::TableSetupColumn("label", ImGuiTableColumnFlags_WidthFixed, DetailsLabelWidth);
		for (int valueColumnIndex = 0; valueColumnIndex < valueColumnCount; ++valueColumnIndex)
		{
			ImGui::TableSetupColumn("value", ImGuiTableColumnFlags_WidthStretch);
		}
		ImGui::TableSetupColumn("utility", ImGuiTableColumnFlags_WidthFixed, DetailsUtilityColumnWidth);

		ImGui::TableNextRow();
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, DetailsRowBackgroundColor());
		ImGui::TableSetColumnIndex(0);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", label);
		return true;
	}

	bool DrawDetailsResetButton(int utilityColumnIndex, bool dirty, const char* tooltip)
	{
		ImGui::TableSetColumnIndex(utilityColumnIndex);
		if (!dirty)
		{
			ImGui::Dummy(ImVec2(DetailsUtilityColumnWidth, DetailsResetButtonSize));
			return false;
		}

		ImGui::PushID("reset");
		const ImVec2 start = ImGui::GetCursorScreenPos();
		const ImVec2 size(DetailsResetButtonSize, DetailsResetButtonSize);
		const bool pressed = ImGui::InvisibleButton("##reset", size);
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 end(start.x + size.x, start.y + size.y);
		if (hovered || active)
		{
			drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::ButtonBackgroundHovered()), 3.0f);
		}

		const ImVec4 iconColor = hovered ? SparkleUiPalette::TextPrimary() : WithAlpha(SparkleUiPalette::TextMuted(), 0.62f);
		const ImU32 iconColorU32 = ImGui::ColorConvertFloat4ToU32(iconColor);
		DrawCenteredGlyph(drawList, start, size, GetEditorIconGlyph(EditorIcon::Reset), iconColorU32);
		if (tooltip != nullptr && tooltip[0] != '\0' && hovered)
		{
			ImGui::SetTooltip("%s", tooltip);
		}

		ImGui::PopID();
		return pressed;
	}

	void DrawDetailsEmptyUtility(int utilityColumnIndex)
	{
		ImGui::TableSetColumnIndex(utilityColumnIndex);
		ImGui::Dummy(ImVec2(DetailsUtilityColumnWidth, PlaceholderIconSize));
	}

	void EndDetailsRow()
	{
		ImGui::EndTable();
		ImGui::Separator();
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar(5);
		ImGui::PopID();
	}
}
