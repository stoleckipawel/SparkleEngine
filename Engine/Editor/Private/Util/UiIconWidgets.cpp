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
	const char* GetEditorIconGlyph(EditorIcon icon) noexcept
	{
		switch (icon)
		{
			case EditorIcon::Folder:
				return EditorIconGlyphs::FontAwesome::Folder;
			case EditorIcon::FolderOpen:
				return EditorIconGlyphs::FontAwesome::FolderOpen;
			case EditorIcon::Camera:
				return EditorIconGlyphs::FontAwesome::Camera;
			case EditorIcon::Light:
				return EditorIconGlyphs::FontAwesome::Light;
			case EditorIcon::DirectionalLight:
				return EditorIconGlyphs::FontAwesome::DirectionalLight;
			case EditorIcon::PointLight:
				return EditorIconGlyphs::FontAwesome::PointLight;
			case EditorIcon::SpotLight:
				return EditorIconGlyphs::FontAwesome::SpotLight;
			case EditorIcon::StaticMesh:
				return EditorIconGlyphs::FontAwesome::StaticMesh;
			case EditorIcon::Material:
				return EditorIconGlyphs::FontAwesome::Material;
			case EditorIcon::EyeVisible:
				return EditorIconGlyphs::FontAwesome::EyeVisible;
			case EditorIcon::EyeHidden:
				return EditorIconGlyphs::FontAwesome::EyeHidden;
			case EditorIcon::Reset:
				return EditorIconGlyphs::FontAwesome::Reset;
			case EditorIcon::Filter:
				return EditorIconGlyphs::FontAwesome::Filter;
			case EditorIcon::Settings:
				return EditorIconGlyphs::FontAwesome::Settings;
			case EditorIcon::Save:
				return EditorIconGlyphs::FontAwesome::Save;
			case EditorIcon::Shader:
				return EditorIconGlyphs::FontAwesome::Shader;
			case EditorIcon::Refresh:
				return EditorIconGlyphs::FontAwesome::Refresh;
			case EditorIcon::Reload:
				return EditorIconGlyphs::FontAwesome::Reload;
			case EditorIcon::Search:
				return EditorIconGlyphs::FontAwesome::Search;
			case EditorIcon::Level:
				return EditorIconGlyphs::FontAwesome::Level;
			case EditorIcon::ViewMode:
				return EditorIconGlyphs::FontAwesome::ViewMode;
			case EditorIcon::ViewLit:
				return EditorIconGlyphs::FontAwesome::ViewLit;
			case EditorIcon::ViewDiffuse:
				return EditorIconGlyphs::FontAwesome::ViewDiffuse;
			case EditorIcon::ViewNormal:
				return EditorIconGlyphs::FontAwesome::ViewNormal;
			case EditorIcon::ViewRoughness:
				return EditorIconGlyphs::FontAwesome::ViewRoughness;
			case EditorIcon::ViewMetallic:
				return EditorIconGlyphs::FontAwesome::ViewMetallic;
			case EditorIcon::ViewEmissive:
				return EditorIconGlyphs::FontAwesome::ViewEmissive;
			case EditorIcon::ViewAmbientOcclusion:
				return EditorIconGlyphs::FontAwesome::ViewAmbientOcclusion;
			case EditorIcon::ViewSubsurfaceColor:
				return EditorIconGlyphs::FontAwesome::ViewSubsurfaceColor;
			case EditorIcon::ViewSubsurfaceStrength:
				return EditorIconGlyphs::FontAwesome::ViewSubsurfaceStrength;
			case EditorIcon::ViewDirectDiffuse:
				return EditorIconGlyphs::FontAwesome::ViewDirectDiffuse;
			case EditorIcon::ViewDirectSpecular:
				return EditorIconGlyphs::FontAwesome::ViewDirectSpecular;
			case EditorIcon::ViewDirectSubsurface:
				return EditorIconGlyphs::FontAwesome::ViewDirectSubsurface;
			case EditorIcon::Cpu:
				return EditorIconGlyphs::FontAwesome::Cpu;
			case EditorIcon::Gpu:
				return EditorIconGlyphs::FontAwesome::Gpu;
			case EditorIcon::Help:
				return EditorIconGlyphs::FontAwesome::Help;
			case EditorIcon::Clear:
				return EditorIconGlyphs::FontAwesome::Clear;
			case EditorIcon::Copy:
				return EditorIconGlyphs::FontAwesome::Copy;
			case EditorIcon::Console:
				return EditorIconGlyphs::FontAwesome::Console;
			case EditorIcon::SourceFile:
				return EditorIconGlyphs::FontAwesome::SourceFile;
			case EditorIcon::Reflection:
				return EditorIconGlyphs::FontAwesome::Reflection;
			case EditorIcon::Disassembly:
				return EditorIconGlyphs::FontAwesome::Disassembly;
			case EditorIcon::CompileRequest:
				return EditorIconGlyphs::FontAwesome::CompileRequest;
			case EditorIcon::Sort:
				return EditorIconGlyphs::FontAwesome::Sort;
			case EditorIcon::None:
			default:
				return EditorIconGlyphs::FontAwesome::Default;
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

	void DrawEditorIcon(EditorIcon icon, const char* tooltip, bool drawBadgeBackground)
	{
		DrawPlaceholderTypeIcon(GetEditorIconGlyph(icon), tooltip, drawBadgeBackground);
	}

	bool DrawEditorIconButton(EditorIcon icon, const char* id, const char* tooltip)
	{
		ImGui::PushID(id);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2.0f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, SparkleUiPalette::ButtonBackgroundHovered());
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, SparkleUiPalette::ButtonBackgroundActive());
		ImGui::PushStyleColor(ImGuiCol_Text, SparkleUiPalette::TextMuted());
		const bool pressed = ImGui::Button(GetEditorIconGlyph(icon), ImVec2(PlaceholderIconSize, PlaceholderIconSize));
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
		constexpr float kVisibilityIconSize = 14.0f;
		const float availableWidth = ImGui::GetContentRegionAvail().x;
		const float horizontalOffset = (std::max) (0.0f, (availableWidth - kVisibilityIconSize) * 0.5f);
		const float verticalOffset = (std::max) (0.0f, (ImGui::GetFrameHeight() - kVisibilityIconSize) * 0.5f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + horizontalOffset);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + verticalOffset);
		return DrawVisibilityIconButton(id, visible);
	}

	void DrawPlaceholderTypeIcon(const char* text, const char* tooltip, bool drawBadgeBackground)
	{
		const ImVec2 size(PlaceholderIconSize, PlaceholderIconSize);
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
		const ImVec2 size(PlaceholderIconSize, PlaceholderIconSize);
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
		DrawCenteredGlyph(
		    drawList,
		    start,
		    size,
		    GetEditorIconGlyph(visible ? EditorIcon::EyeVisible : EditorIcon::EyeHidden),
		    iconColorU32);

		if (hovered)
		{
			ImGui::SetTooltip("%s", visible ? "Visible" : "Hidden");
		}
		ImGui::PopID();
		return pressed;
	}

}
