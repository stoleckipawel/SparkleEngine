#include "PCH.h"
#include "Util/UiUtil.h"

#include "Style/SparkleUiPalette.h"
#include "Style/SparkleUiTheme.h"

#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>

#include <imgui.h>

#include "Util/UiWidgetPrimitives.h"

namespace UiUtil
{
	// Semantic icons map directly to the Font Awesome Free Solid 6.7.1 font asset.
	const char* GetEditorIconGlyph(EditorIcon icon) noexcept
	{
		switch (icon)
		{
			case EditorIcon::Folder:
				return "\xef\x81\xbb";
			case EditorIcon::FolderOpen:
				return "\xef\x81\xbc";
			case EditorIcon::Camera:
				return "\xef\x80\xb0";
			case EditorIcon::Light:
				return "\xef\x83\xa7";
			case EditorIcon::DirectionalLight:
				return "\xef\x86\x85";
			case EditorIcon::PointLight:
				return "\xef\x86\x92";
			case EditorIcon::SpotLight:
				return "\xef\x85\x80";
			case EditorIcon::RectLight:
				return "\xef\x83\x88";
			case EditorIcon::Sky:
				return "\xef\x83\x82";
			case EditorIcon::StaticMesh:
				return "\xef\x86\xb2";
			case EditorIcon::SkinnedMesh:
				return "\xef\x86\x83";
			case EditorIcon::Material:
				return "\xef\x94\xbf";
			case EditorIcon::EyeVisible:
				return "\xef\x81\xae";
			case EditorIcon::EyeHidden:
				return "\xef\x81\xb0";
			case EditorIcon::Reset:
				return "\xef\x83\xa2";
			case EditorIcon::Filter:
				return "\xef\x82\xb0";
			case EditorIcon::Settings:
				return "\xef\x80\x93";
			case EditorIcon::Save:
				return "\xef\x83\x87";
			case EditorIcon::Shader:
				return "\xef\x8b\x9b";
			case EditorIcon::Refresh:
				return "\xef\x80\xa1";
			case EditorIcon::Reload:
				return "\xef\x87\x80";
			case EditorIcon::Search:
				return "\xef\x80\x82";
			case EditorIcon::Level:
				return "\xef\x89\xb9";
			case EditorIcon::ViewMode:
				return "\xef\x97\xbd";
			case EditorIcon::ViewLit:
				return "\xef\x86\x85";
			case EditorIcon::ViewDiffuse:
				return "\xef\x94\xbf";
			case EditorIcon::ViewNormal:
				return "\xef\x81\x87";
			case EditorIcon::ViewRoughness:
				return "\xef\x9b\xbc";
			case EditorIcon::ViewMetallic:
				return "\xef\x81\xb6";
			case EditorIcon::ViewEmissive:
				return "\xef\x81\xad";
			case EditorIcon::ViewAmbientOcclusion:
				return "\xef\x81\x82";
			case EditorIcon::ViewSubsurfaceColor:
				return "\xef\x81\x83";
			case EditorIcon::ViewSubsurfaceStrength:
				return "\xef\x9d\xb3";
			case EditorIcon::ViewDirectDiffuse:
				return "\xef\x83\xa7";
			case EditorIcon::ViewDirectSpecular:
				return "\xef\x8e\xa5";
			case EditorIcon::ViewDirectSubsurface:
				return "\xef\x97\x92";
			case EditorIcon::Cpu:
				return "\xef\x8b\x9b";
			case EditorIcon::Gpu:
				return "\xef\x84\x88";
			case EditorIcon::Help:
				return "\xef\x81\x99";
			case EditorIcon::Clear:
				return "\xef\x87\xb8";
			case EditorIcon::Copy:
				return "\xef\x83\x85";
			case EditorIcon::Console:
				return "\xef\x84\xa0";
			case EditorIcon::SourceFile:
				return "\xef\x87\x89";
			case EditorIcon::Reflection:
				return "\xef\x82\xae";
			case EditorIcon::Disassembly:
				return "\xef\x84\xa0";
			case EditorIcon::CompileRequest:
				return "\xef\x91\xad";
			case EditorIcon::Sort:
				return "\xef\x85\xa0";
			case EditorIcon::None:
			default:
				return "\xef\x81\x99";
		}
	}

	std::string MakeIconLabel(EditorIcon icon, const char* label)
	{
		std::string result = GetEditorIconGlyph(icon);
		if (label != nullptr && label[0] != '\0')
		{
			result += ' ';
			result += label;
		}
		return result;
	}

	bool MatchesDetailsFilter(const std::string& filterText, const char* title, const char* keywords) noexcept
	{
		return filterText.empty() || Strings::ContainsIgnoreCase(title, filterText) || Strings::ContainsIgnoreCase(keywords, filterText);
	}

	ImU32 WithAlphaU32(ImVec4 color, float alpha) noexcept
	{
		color.w *= alpha;
		return ImGui::ColorConvertFloat4ToU32(color);
	}

	bool DrawEditorIconButton(EditorIcon icon, const char* id, const char* tooltip)
	{
		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SparkleUiPalette::ButtonBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, SparkleUiPalette::ButtonBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
		const bool pressed = ImGui::Button(GetEditorIconGlyph(icon), ImVec2(EditorIconSize, EditorIconSize));
		if (tooltip != nullptr && tooltip[0] != '\0' && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();
		ImGui::PopID();
		return pressed;
	}

	bool DrawFilterChip(const char* label, bool active) noexcept
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5.0f, 2.0f));
		if (active)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, SparkleUiPalette::ButtonBackgroundActive());
			ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextPrimary());
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Button, SparkleUiPalette::ButtonBackground());
			ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
		}

		ImGui::PushID("FilterChip");
		const bool pressed = ImGui::SmallButton(label);
		ImGui::PopID();
		ImGui::PopStyleColor(2);
		ImGui::PopStyleVar();
		return pressed;
	}

	void DrawMutedText(const char* text, float alpha) noexcept
	{
		ImVec4 color = SparkleUiPalette::TextMuted();
		color.w *= alpha;
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(text);
		ImGui::PopStyleColor();
	}

	bool DrawCenteredVisibilityIconButton(const char* id, bool visible) noexcept
	{
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float horizontalOffset = (std::max) (0.0f, (availableWidth - EditorIconSize) * 0.5f);
		const float verticalOffset = (std::max) (0.0f, (ImGui::GetFrameHeight() - EditorIconSize) * 0.5f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalOffset);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);
		return DrawVisibilityIconButton(id, visible);
	}

	void DrawEditorIcon(EditorIcon icon, const char* tooltip, bool drawBadgeBackground)
	{
		const char* text = GetEditorIconGlyph(icon);
		const ImVec2 size(EditorIconSize, EditorIconSize);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		ImGui::InvisibleButton("##placeholder_type_icon", size);

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 end(start.x + size.x, start.y + size.y);
		if (drawBadgeBackground)
		{
			drawList->AddRectFilled(start, end, SparkleUiPalette::SceneOutlinerBadgeBackground(), 3.0f);
			drawList->AddRect(start, end, SparkleUiPalette::PanelHeaderBorder(), 3.0f, 0, 1.0f);
		}

		if (text != nullptr && text[0] != '\0')
		{
			const ImVec2 textSize = ImGui::CalcTextSize(text);
			const ImVec2 textPos(start.x + ((size.x - textSize.x) * 0.5f), start.y + ((size.y - textSize.y) * 0.5f));
			drawList->AddText(textPos, SparkleUiPalette::SceneOutlinerBadgeText(), text);
		}

		if (tooltip != nullptr && tooltip[0] != '\0' && ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s", tooltip);
		}
	}

	bool DrawVisibilityIconButton(const char* id, bool visible)
	{
		ImGui::PushID(id);
		const ImVec2 start = ImGui::GetCursorScreenPos();
		const ImVec2 size(EditorIconSize, EditorIconSize);
		const bool pressed = ImGui::InvisibleButton("##visibility", size);
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		const ImVec2 end(start.x + size.x, start.y + size.y);
		if (hovered || active)
		{
			drawList->AddRectFilled(start, end, ImGui::ColorConvertFloat4ToU32(SparkleUiPalette::ButtonBackgroundHovered()), 3.0f);
		}

		const ImVec4 iconColor = visible ? WithAlpha(SparkleUiPalette::TextMuted(), 0.58f) : SparkleUiPalette::AccentStrong();
		const ImU32 iconColorU32 = ImGui::ColorConvertFloat4ToU32(iconColor);
		DrawCenteredGlyph(drawList, start, size, GetEditorIconGlyph(visible ? EditorIcon::EyeVisible : EditorIcon::EyeHidden), iconColorU32);

		if (hovered)
		{
			ImGui::SetTooltip("%s", visible ? "Visible" : "Hidden");
		}
		ImGui::PopID();
		return pressed;
	}

}
