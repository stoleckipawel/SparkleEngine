#include "PCH.h"
#include "Panels/ViewportTopPanel.h"

#include "Level/Level.h"
#include "Level/LevelManager.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"

#include <imgui.h>

#include <cstring>
#include <cstdio>
#include <string>

class ViewportTopPanelOperations final
{
  public:
	static UiUtil::EditorIcon GetViewModeIcon(RenderViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case RenderViewMode::Lit:
				return UiUtil::EditorIcon::ViewLit;
			case RenderViewMode::Wireframe:
				return UiUtil::EditorIcon::ViewMode;
			case RenderViewMode::GBufferDiffuse:
				return UiUtil::EditorIcon::ViewDiffuse;
			case RenderViewMode::GBufferNormal:
				return UiUtil::EditorIcon::ViewNormal;
			case RenderViewMode::GBufferRoughness:
				return UiUtil::EditorIcon::ViewRoughness;
			case RenderViewMode::GBufferMetallic:
				return UiUtil::EditorIcon::ViewMetallic;
			case RenderViewMode::GBufferEmissive:
				return UiUtil::EditorIcon::ViewEmissive;
			case RenderViewMode::GBufferAmbientOcclusion:
				return UiUtil::EditorIcon::ViewAmbientOcclusion;
			case RenderViewMode::GBufferSubsurfaceColor:
				return UiUtil::EditorIcon::ViewSubsurfaceColor;
			case RenderViewMode::GBufferSubsurfaceStrength:
				return UiUtil::EditorIcon::ViewSubsurfaceStrength;
			case RenderViewMode::DirectDiffuse:
				return UiUtil::EditorIcon::ViewDirectDiffuse;
			case RenderViewMode::DirectSpecular:
				return UiUtil::EditorIcon::ViewDirectSpecular;
			case RenderViewMode::DirectSubsurface:
				return UiUtil::EditorIcon::ViewDirectSubsurface;
			case RenderViewMode::IndirectDiffuse:
				return UiUtil::EditorIcon::ViewDirectDiffuse;
			case RenderViewMode::IndirectSpecular:
				return UiUtil::EditorIcon::ViewDirectSpecular;
			case RenderViewMode::InstanceGroups:
				return UiUtil::EditorIcon::ViewMode;
			case RenderViewMode::Count:
				break;
		}

		return UiUtil::EditorIcon::ViewLit;
	}
};

ViewportTopPanel::ViewportTopPanel(LevelManager* levelManager) noexcept
{
	SetLevelManager(levelManager);
}

void ViewportTopPanel::SetLevelManager(LevelManager* levelManager) noexcept
{
	m_levelManager = levelManager;
}

void ViewportTopPanel::SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept
{
	m_leftPixels = leftPixels;
	m_topPixels = topPixels;
	m_widthPixels = widthPixels;
}

const char* ViewportTopPanel::GetViewModeLabel(RenderViewMode viewMode) noexcept
{
	switch (viewMode)
	{
		case RenderViewMode::Lit:
			return "Lit";
		case RenderViewMode::Wireframe:
			return "Wireframe";
		case RenderViewMode::GBufferDiffuse:
			return "GBuffer Diffuse";
		case RenderViewMode::GBufferNormal:
			return "GBuffer Normal";
		case RenderViewMode::GBufferRoughness:
			return "GBuffer Roughness";
		case RenderViewMode::GBufferMetallic:
			return "GBuffer Metallic";
		case RenderViewMode::GBufferEmissive:
			return "GBuffer Emissive";
		case RenderViewMode::GBufferAmbientOcclusion:
			return "GBuffer Ambient Occlusion";
		case RenderViewMode::GBufferSubsurfaceColor:
			return "GBuffer Subsurface Color";
		case RenderViewMode::GBufferSubsurfaceStrength:
			return "GBuffer Subsurface Strength";
		case RenderViewMode::DirectDiffuse:
			return "Direct Diffuse";
		case RenderViewMode::DirectSpecular:
			return "Direct Specular";
		case RenderViewMode::DirectSubsurface:
			return "Direct Subsurface";
		case RenderViewMode::IndirectDiffuse:
			return "Indirect Diffuse";
		case RenderViewMode::IndirectSpecular:
			return "Indirect Specular";
		case RenderViewMode::InstanceGroups:
			return "Instance Groups";
		case RenderViewMode::Count:
			break;
	}

	return "Lit";
}

void ViewportTopPanel::DrawViewModeCategory(const char* label) noexcept
{
	ImGui::Spacing();
	ImGui::Separator();
	UiUtil::EditorIcon icon = UiUtil::EditorIcon::ViewMode;
	if (std::strcmp(label, "Lighting") == 0)
	{
		icon = UiUtil::EditorIcon::Light;
	}
	const std::string categoryLabel = UiUtil::MakeIconLabel(icon, label);
	ImGui::TextDisabled("%s", categoryLabel.c_str());
}

void ViewportTopPanel::DrawViewModeOption(RenderViewMode option, RenderViewMode currentViewMode) noexcept
{
	const bool selected = option == currentViewMode;
	const std::string optionLabel = UiUtil::MakeIconLabel(ViewportTopPanelOperations::GetViewModeIcon(option), GetViewModeLabel(option));
	if (ImGui::Selectable(optionLabel.c_str(), selected))
	{
		CVarRenderViewMode.Set(option);
	}

	if (selected)
	{
		ImGui::SetItemDefaultFocus();
	}
}

void ViewportTopPanel::BuildLevelName() const noexcept
{
	const LevelAsset* activeLevel = m_levelManager != nullptr ? m_levelManager->GetActiveLevel() : nullptr;
	const std::string activeLevelName = activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string("<None>");

	ImGui::AlignTextToFramePadding();
	const std::string levelLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Level, "Level");
	ImGui::TextDisabled("%s", levelLabel.c_str());
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(activeLevelName.c_str());
}

void ViewportTopPanel::BuildViewModeCombo(bool disableInteraction) noexcept
{
	RenderViewMode currentViewMode = CVarRenderViewMode.Get();
	if (currentViewMode >= RenderViewMode::Count)
	{
		currentViewMode = RenderViewMode::Lit;
	}

	ImGui::AlignTextToFramePadding();
	const std::string viewModeLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::ViewMode, "Viewmode");
	ImGui::TextDisabled("%s", viewModeLabel.c_str());
	ImGui::SameLine();
	ImGui::SetNextItemWidth(180.0f);
	ImGui::BeginDisabled(disableInteraction);
	const std::string previewLabel = UiUtil::MakeIconLabel(ViewportTopPanelOperations::GetViewModeIcon(currentViewMode), GetViewModeLabel(currentViewMode));
	if (ImGui::BeginCombo("##ViewportViewMode", previewLabel.c_str()))
	{
		DrawViewModeOption(RenderViewMode::Lit, currentViewMode);
		DrawViewModeOption(RenderViewMode::Wireframe, currentViewMode);
		DrawViewModeOption(RenderViewMode::InstanceGroups, currentViewMode);

		DrawViewModeCategory("GBuffer");
		ImGui::Indent(8.0f);
		DrawViewModeOption(RenderViewMode::GBufferDiffuse, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferNormal, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferRoughness, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferMetallic, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferEmissive, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferAmbientOcclusion, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferSubsurfaceColor, currentViewMode);
		DrawViewModeOption(RenderViewMode::GBufferSubsurfaceStrength, currentViewMode);
		ImGui::Unindent(8.0f);

		DrawViewModeCategory("Lighting");
		ImGui::Indent(8.0f);
		DrawViewModeOption(RenderViewMode::DirectDiffuse, currentViewMode);
		DrawViewModeOption(RenderViewMode::DirectSpecular, currentViewMode);
		DrawViewModeOption(RenderViewMode::DirectSubsurface, currentViewMode);
		DrawViewModeOption(RenderViewMode::IndirectDiffuse, currentViewMode);
		DrawViewModeOption(RenderViewMode::IndirectSpecular, currentViewMode);
		ImGui::Unindent(8.0f);

		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}

void ViewportTopPanel::BuildPerformanceStats() const noexcept
{
	const ImGuiIO& io = ImGui::GetIO();
	char statsText[64] = {};
	std::snprintf(statsText, sizeof(statsText), "%.1f FPS  %.2f ms", io.Framerate, io.DeltaTime * 1000.0f);

	const ImGuiStyle& style = ImGui::GetStyle();
	const float statsWidth = ImGui::CalcTextSize(statsText).x;
	const float rightAlignedX = ImGui::GetWindowWidth() - style.WindowPadding.x - statsWidth;
	if (rightAlignedX > ImGui::GetCursorPosX() + style.ItemSpacing.x)
	{
		ImGui::SameLine();
		ImGui::SetCursorPosX(rightAlignedX);
	}
	else
	{
		ImGui::SameLine();
	}

	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("%s", statsText);
}

void ViewportTopPanel::BuildUI(bool disableInteraction) noexcept
{
	if (m_widthPixels <= 0.0f)
	{
		m_heightPixels = 0.0f;
		return;
	}

	const ImVec2 windowPadding(10.0f, 4.0f);
	m_heightPixels = ImGui::GetFrameHeight() + (windowPadding.y * 2.0f);

	ImGui::SetNextWindowPos(ImVec2(m_leftPixels, m_topPixels), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(m_widthPixels, m_heightPixels), ImGuiCond_Always);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, windowPadding);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, SparkleUiPalette::TitleBarBackground());
	ImGui::PushStyleColor(ImGuiCol_FrameBg, SparkleUiPalette::FrameBackground());
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, SparkleUiPalette::FrameBackgroundHovered());
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, SparkleUiPalette::FrameBackgroundActive());
	ImGui::PushStyleColor(ImGuiCol_Header, SparkleUiPalette::HeaderBackground());
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, SparkleUiPalette::HeaderBackgroundHovered());
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, SparkleUiPalette::HeaderBackgroundActive());

	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoMove;
	windowFlags |= ImGuiWindowFlags_NoResize;
	windowFlags |= ImGuiWindowFlags_NoCollapse;
	windowFlags |= ImGuiWindowFlags_NoTitleBar;
	windowFlags |= ImGuiWindowFlags_NoScrollbar;
	windowFlags |= ImGuiWindowFlags_NoScrollWithMouse;
	windowFlags |= ImGuiWindowFlags_NoSavedSettings;

	if (!ImGui::Begin("Viewport Top Panel", nullptr, windowFlags))
	{
		ImGui::End();
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar(2);
		return;
	}

	BuildLevelName();
	ImGui::SameLine(0.0f, 14.0f);
	ImGui::AlignTextToFramePadding();
	ImGui::TextDisabled("|");
	ImGui::SameLine(0.0f, 14.0f);
	BuildViewModeCombo(disableInteraction);
	BuildPerformanceStats();

	ImDrawList* drawList = ImGui::GetWindowDrawList();
	const ImVec2 windowMin = ImGui::GetWindowPos();
	const ImVec2 windowMax(windowMin.x + ImGui::GetWindowWidth(), windowMin.y + ImGui::GetWindowHeight());
	drawList->AddLine(
	    ImVec2(windowMin.x, windowMax.y - 1.0f),
	    ImVec2(windowMax.x, windowMax.y - 1.0f),
	    SparkleUiPalette::PanelHeaderBorder(),
	    1.0f);

	ImGui::End();
	ImGui::PopStyleColor(7);
	ImGui::PopStyleVar(2);
}
