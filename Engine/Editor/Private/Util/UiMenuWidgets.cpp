#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiPalette.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

namespace UiUtil
{
	constexpr float menuHorizontalPadding = 0.6f;
	constexpr float menuVerticalPadding = 0.4f;
	constexpr float menuHorizontalSpacing = 0.5f;
	constexpr float menuVerticalSpacing = 0.4f;
	constexpr float menuIconOffset = 0.2f;
	constexpr float menuLabelOffset = 1.9f;
	constexpr float menuCheckColumnWidth = 1.5f;
	constexpr float menuArrowColumnWidth = 1.2f;
	constexpr float menuHeadingScale = 0.8f;
	constexpr float menuHeadingTracking = 0.075f;
	constexpr float menuHeadingLineGap = 0.7f;
	constexpr float menuHeadingLineWidth = 1.5f;

	static const char* NextMenuHeadingGlyph(const char* character)
	{
		const char* next = character + 1;
		while ((static_cast<unsigned char>(*next) & 0xc0u) == 0x80u)
		{
			++next;
		}
		return next;
	}

	static void DrawMenuRowContent(ImDrawList& drawList, ImVec2 position, const char* label, EditorIcon icon, bool hasCheckColumn)
	{
		const float fontSize = ImGui::GetFontSize();
		const float checkOffset = hasCheckColumn ? menuCheckColumnWidth : 0.0f;
		if (icon != UiUtil::EditorIcon::None)
		{
			drawList.AddText(
			    ImVec2(position.x + (menuIconOffset + checkOffset) * fontSize, position.y),
			    ImGui::GetColorU32(SparkleUiPalette::Menu().Icon),
			    UiUtil::GetEditorIconGlyph(icon));
		}
		const char* labelEnd = std::strstr(label, "##");
		drawList.AddText(
		    ImVec2(position.x + (menuLabelOffset + checkOffset) * fontSize, position.y),
		    ImGui::GetColorU32(ImGuiCol_Text),
		    label,
		    labelEnd);
	}
	MenuStyleScope::MenuStyleScope()
	{
		const float fontSize = ImGui::GetFontSize();
		ImGui::PushStyleVar(
		    ImGuiStyleVar_WindowPadding,
		    ImVec2(std::round(menuHorizontalPadding * fontSize), std::round(menuVerticalPadding * fontSize)));
		ImGui::PushStyleVar(
		    ImGuiStyleVar_ItemSpacing,
		    ImVec2(std::round(menuHorizontalSpacing * fontSize), std::round(menuVerticalSpacing * fontSize)));
		ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_PopupRounding, 0.12f * fontSize);
		const SparkleUiPalette::MenuColors& colors = SparkleUiPalette::Menu();
		ImGui::PushStyleColor(ImGuiCol_PopupBg, colors.Background);
		ImGui::PushStyleColor(ImGuiCol_Text, colors.Text);
		ImGui::PushStyleColor(ImGuiCol_TextDisabled, colors.MutedText);
		ImGui::PushStyleColor(ImGuiCol_Header, colors.Hovered);
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, colors.Hovered);
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, colors.Active);
		ImGui::PushStyleColor(ImGuiCol_Separator, colors.Divider);
	}

	MenuStyleScope::~MenuStyleScope()
	{
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar(4);
	}

	float MeasureMenuRow(const char* label, MenuRowKind kind)
	{
		float columns = menuLabelOffset + 2.0f * menuHorizontalPadding;
		if (kind == MenuRowKind::Toggle)
		{
			columns += menuCheckColumnWidth;
		}
		else if (kind == MenuRowKind::Submenu)
		{
			columns += menuArrowColumnWidth;
		}
		return std::ceil(ImGui::CalcTextSize(label, nullptr, true).x + columns * ImGui::GetFontSize());
	}

	float MeasureMenuSection(const char* label)
	{
		const float fontSize = ImGui::GetFontSize();
		float width = (menuHeadingLineGap + menuHeadingLineWidth + 2.0f * menuHorizontalPadding) * fontSize;
		for (const char* character = label; *character != '\0'; character = NextMenuHeadingGlyph(character))
		{
			width += ImGui::GetFont()
			             ->CalcTextSizeA(
			                 fontSize * menuHeadingScale,
			                 (std::numeric_limits<float>::max)(),
			                 0.0f,
			                 character,
			                 NextMenuHeadingGlyph(character))
			             .x
			    + menuHeadingTracking * fontSize;
		}
		return std::ceil(width);
	}

	bool BeginMenuPopup(const char* id, ImVec2 anchor, float width)
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0.0f), ImVec2(width, ImGui::GetIO().DisplaySize.y));
		ImGui::SetNextWindowPos(ImVec2(anchor.x, anchor.y + 0.15f * ImGui::GetFontSize()), ImGuiCond_Appearing);
		return ImGui::BeginPopup(id);
	}

	bool BeginMenu(const char* label, EditorIcon icon, float childWidth, bool enabled)
	{
		const ImVec2 position = ImGui::GetCursorScreenPos();
		const float width = ImGui::GetContentRegionAvail().x;
		const float fontSize = ImGui::GetFontSize();
		ImDrawList& drawList = *ImGui::GetWindowDrawList();
		ImGui::SetNextWindowSizeConstraints(ImVec2(childWidth, 0.0f), ImVec2(childWidth, ImGui::GetIO().DisplaySize.y));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		const bool open = ImGui::BeginMenu(label, enabled);
		ImGui::PopStyleColor();
		ImGui::BeginDisabled(!enabled);
		DrawMenuRowContent(drawList, position, label, icon, false);
		const float arrowX = position.x + width - menuHorizontalPadding * fontSize;
		const float centerY = position.y + 0.5f * fontSize;
		const ImU32 arrowColor = ImGui::GetColorU32(SparkleUiPalette::Menu().Icon);
		drawList
		    .AddLine(ImVec2(arrowX, centerY - 0.25f * fontSize), ImVec2(arrowX + 0.25f * fontSize, centerY), arrowColor, 0.09f * fontSize);
		drawList
		    .AddLine(ImVec2(arrowX + 0.25f * fontSize, centerY), ImVec2(arrowX, centerY + 0.25f * fontSize), arrowColor, 0.09f * fontSize);
		ImGui::EndDisabled();
		return open;
	}

	bool DrawMenuItem(const char* label, EditorIcon icon, MenuCheckState checkState, ImGuiSelectableFlags flags)
	{
		const ImVec2 position = ImGui::GetCursorScreenPos();
		const bool hasCheckColumn = checkState != MenuCheckState::Hidden;
		const float minimumWidth =
		    MeasureMenuRow(label, hasCheckColumn ? MenuRowKind::Toggle : MenuRowKind::Action) - 2.0f * ImGui::GetStyle().WindowPadding.x;
		const float width = (std::max) (minimumWidth, ImGui::GetContentRegionAvail().x);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		const bool pressed = ImGui::Selectable(label, false, flags, ImVec2(width, ImGui::GetFontSize()));
		ImGui::PopStyleColor();
		ImDrawList& drawList = *ImGui::GetWindowDrawList();
		DrawMenuRowContent(drawList, position, label, icon, hasCheckColumn);
		const float fontSize = ImGui::GetFontSize();
		const float centerY = position.y + 0.5f * fontSize;
		const ImU32 color = ImGui::GetColorU32(ImGuiCol_Text);
		if (checkState == MenuCheckState::Checked)
		{
			drawList.AddLine(
			    ImVec2(position.x + 0.1f * fontSize, centerY),
			    ImVec2(position.x + 0.35f * fontSize, centerY + 0.25f * fontSize),
			    color,
			    0.11f * fontSize);
			drawList.AddLine(
			    ImVec2(position.x + 0.35f * fontSize, centerY + 0.25f * fontSize),
			    ImVec2(position.x + 0.85f * fontSize, centerY - 0.3f * fontSize),
			    color,
			    0.11f * fontSize);
		}
		else if (checkState == MenuCheckState::Mixed)
		{
			drawList.AddLine(
			    ImVec2(position.x + 0.1f * fontSize, centerY),
			    ImVec2(position.x + 0.85f * fontSize, centerY),
			    color,
			    0.11f * fontSize);
		}
		return pressed;
	}

	void DrawMenuSection(const char* label)
	{
		const float fontSize = ImGui::GetFontSize();
		const ImVec2 position = ImGui::GetCursorScreenPos();
		const float width = ImGui::GetContentRegionAvail().x;
		ImDrawList& drawList = *ImGui::GetWindowDrawList();
		const float headingSize = fontSize * menuHeadingScale;
		float textX = position.x;
		for (const char* character = label; *character != '\0'; character = NextMenuHeadingGlyph(character))
		{
			const char* glyphEnd = NextMenuHeadingGlyph(character);
			drawList.AddText(
			    ImGui::GetFont(),
			    headingSize,
			    ImVec2(textX, position.y + 0.1f * fontSize),
			    ImGui::GetColorU32(SparkleUiPalette::Menu().Heading),
			    character,
			    glyphEnd);
			textX += ImGui::GetFont()->CalcTextSizeA(headingSize, (std::numeric_limits<float>::max)(), 0.0f, character, glyphEnd).x
			    + menuHeadingTracking * fontSize;
		}
		const float lineStart = textX + menuHeadingLineGap * fontSize;
		if (lineStart < position.x + width)
		{
			drawList.AddLine(
			    ImVec2(lineStart, position.y + 0.5f * fontSize),
			    ImVec2(position.x + width, position.y + 0.5f * fontSize),
			    ImGui::GetColorU32(SparkleUiPalette::Menu().Divider),
			    fontSize / 14.0f);
		}
		ImGui::Dummy(ImVec2(width, std::round(0.9f * fontSize)));
	}
}
