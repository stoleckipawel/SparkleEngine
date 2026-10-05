#include "PCH.h"
#include "Panels/ViewportTopPanel.h"

#include "Level/Level.h"
#include "Level/LevelSession.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Settings/EngineRenderingSettingsController.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"
#include "Viewport/ViewportCameraProperties.h"
#include "Viewport/EditorViewportSession.h"
#include "Viewport/ViewportShowMenu.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <string>
#include <string_view>

struct ViewModePresentation final
{
	RenderViewMode Mode;
	UiUtil::EditorIcon Icon;
	const char* Label;
	const char* Category;
};

static constexpr auto viewModePresentations = std::to_array<ViewModePresentation>(
    {{RenderViewMode::Lit, UiUtil::EditorIcon::ViewLit, "Lit", ""},
        {RenderViewMode::ReferencePathTracer, UiUtil::EditorIcon::ViewLit, "Reference Path Tracer", ""},
        {RenderViewMode::Wireframe, UiUtil::EditorIcon::ViewMode, "Wireframe", ""},
        {RenderViewMode::GpuSceneInstances, UiUtil::EditorIcon::ViewMode, "GPU Scene Instances", ""},
        {RenderViewMode::GBufferDiffuse, UiUtil::EditorIcon::ViewDiffuse, "GBuffer Diffuse", "GBuffer"},
        {RenderViewMode::GBufferWorldNormal, UiUtil::EditorIcon::ViewNormal, "GBuffer World Normal", "GBuffer"},
        {RenderViewMode::GBufferWorldTangent, UiUtil::EditorIcon::ViewNormal, "GBuffer World Tangent", "GBuffer"},
        {RenderViewMode::GBufferRoughness, UiUtil::EditorIcon::ViewRoughness, "GBuffer Roughness", "GBuffer"},
        {RenderViewMode::GBufferMetallic, UiUtil::EditorIcon::ViewMetallic, "GBuffer Metallic", "GBuffer"},
        {RenderViewMode::GBufferEmissive, UiUtil::EditorIcon::ViewEmissive, "GBuffer Emissive", "GBuffer"},
        {RenderViewMode::GBufferAmbientOcclusion, UiUtil::EditorIcon::ViewAmbientOcclusion, "GBuffer Ambient Occlusion", "GBuffer"},
        {RenderViewMode::GBufferSubsurfaceColor, UiUtil::EditorIcon::ViewSubsurfaceColor, "GBuffer Subsurface Color", "GBuffer"},
        {RenderViewMode::GBufferSubsurfaceStrength, UiUtil::EditorIcon::ViewSubsurfaceStrength, "GBuffer Subsurface Strength", "GBuffer"},
        {RenderViewMode::DirectDiffuse, UiUtil::EditorIcon::ViewDirectDiffuse, "Direct Diffuse", "Lighting"},
        {RenderViewMode::DirectSpecular, UiUtil::EditorIcon::ViewDirectSpecular, "Direct Specular", "Lighting"},
        {RenderViewMode::DirectSubsurface, UiUtil::EditorIcon::ViewDirectSubsurface, "Direct Subsurface", "Lighting"},
        {RenderViewMode::IndirectDiffuse, UiUtil::EditorIcon::ViewDirectDiffuse, "Indirect Diffuse", "Lighting"},
        {RenderViewMode::IndirectSpecular, UiUtil::EditorIcon::ViewDirectSpecular, "Indirect Specular", "Lighting"}});

static_assert(viewModePresentations.size() == static_cast<std::size_t>(RenderViewMode::Count));

static const ViewModePresentation& DescribeViewMode(RenderViewMode viewMode) noexcept
{
	const auto found = std::ranges::find(viewModePresentations, viewMode, &ViewModePresentation::Mode);
	return found != viewModePresentations.end() ? *found : viewModePresentations.front();
}

ViewportTopPanel::ViewportTopPanel(
    LevelSession* levelSession,
    EngineRenderingSettingsController* renderingSettings,
    EditorViewportSession* viewportSession,
    const CVarControlExecutor* consoleVariables) noexcept :
    m_renderingSettings(renderingSettings),
    m_viewportSession(viewportSession),
    m_consoleVariables(consoleVariables)
{
	SetLevelSession(levelSession);
}

ViewportTopPanel::~ViewportTopPanel() noexcept = default;

void ViewportTopPanel::SetLevelSession(LevelSession* levelSession) noexcept
{
	m_levelSession = levelSession;
}

void ViewportTopPanel::SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept
{
	m_leftPixels = leftPixels;
	m_topPixels = topPixels;
	m_widthPixels = widthPixels;
}

static void DrawViewModeCategory(const char* label) noexcept
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

static void DrawViewModeOption(
    EditorViewportSession* viewportSession,
    const ViewModePresentation& option,
    RenderViewMode currentViewMode) noexcept
{
	const bool selected = option.Mode == currentViewMode;
	const std::string optionLabel = UiUtil::MakeIconLabel(option.Icon, option.Label);
	if (ImGui::Selectable(optionLabel.c_str(), selected))
	{
		if (viewportSession != nullptr)
		{
			viewportSession->SetViewMode(option.Mode);
		}
	}

	if (selected)
	{
		ImGui::SetItemDefaultFocus();
	}
}

void ViewportTopPanel::BuildLevelName(bool compact) const noexcept
{
	const LevelAsset* activeLevel = m_levelSession != nullptr ? m_levelSession->GetActiveLevel() : nullptr;
	const std::string activeLevelName = activeLevel != nullptr ? std::string(activeLevel->GetName()) : std::string("<None>");

	ImGui::AlignTextToFramePadding();
	if (compact)
	{
		const std::string compactLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Level, activeLevelName.c_str());
		ImGui::TextUnformatted(compactLabel.c_str());
		return;
	}

	const std::string levelLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::Level, "Level");
	ImGui::TextDisabled("%s", levelLabel.c_str());
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(activeLevelName.c_str());
}

void ViewportTopPanel::BuildViewModeCombo(bool disableInteraction, bool compact) noexcept
{
	RenderViewMode currentViewMode = m_viewportSession != nullptr ? m_viewportSession->GetViewMode() : RenderViewMode::Lit;
	if (currentViewMode >= RenderViewMode::Count)
	{
		currentViewMode = RenderViewMode::Lit;
	}

	if (!compact)
	{
		ImGui::AlignTextToFramePadding();
		const std::string viewModeLabel = UiUtil::MakeIconLabel(UiUtil::EditorIcon::ViewMode, "Viewmode");
		ImGui::TextDisabled("%s", viewModeLabel.c_str());
		ImGui::SameLine();
	}
	ImGui::SetNextItemWidth(compact ? 145.0f : 180.0f);
	ImGui::BeginDisabled(disableInteraction);
	const ViewModePresentation& currentPresentation = DescribeViewMode(currentViewMode);
	const std::string previewLabel = UiUtil::MakeIconLabel(currentPresentation.Icon, currentPresentation.Label);
	if (ImGui::BeginCombo("##ViewportViewMode", previewLabel.c_str()))
	{
		std::string_view category;
		for (const ViewModePresentation& option : viewModePresentations)
		{
			if (category != option.Category)
			{
				if (!category.empty())
				{
					ImGui::Unindent(8.0f);
				}
				category = option.Category;
				if (!category.empty())
				{
					DrawViewModeCategory(option.Category);
					ImGui::Indent(8.0f);
				}
			}
			DrawViewModeOption(m_viewportSession, option, currentViewMode);
		}
		if (!category.empty())
		{
			ImGui::Unindent(8.0f);
		}

		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}

void ViewportTopPanel::BuildRightControls(bool disableInteraction, bool compact) noexcept
{
	const ImGuiIO& io = ImGui::GetIO();
	char statsText[64] = {};
	std::snprintf(statsText, sizeof(statsText), "%.1f FPS  %.2f ms", io.Framerate, io.DeltaTime * 1000.0f);

	const ImGuiStyle& style = ImGui::GetStyle();
	const bool showStats = !compact;
	const float statsWidth = showStats ? ImGui::CalcTextSize(statsText).x : 0.0f;
	const CameraProjectionKind projectionKind =
	    m_viewportSession != nullptr ? m_viewportSession->GetSettings().ProjectionKind : CameraProjectionKind::Perspective;
	const char* projectionLabel = projectionKind == CameraProjectionKind::Orthographic ? "Orthographic" : "Perspective";
	const std::string cameraText = compact ? UiUtil::GetEditorIconGlyph(UiUtil::EditorIcon::Camera)
	                                       : UiUtil::MakeIconLabel(UiUtil::EditorIcon::Camera, projectionLabel);
	const std::string cameraLabel = cameraText + "##ViewportCameraPropertiesButton";
	const float cameraButtonWidth = ImGui::CalcTextSize(cameraText.c_str()).x + style.FramePadding.x * 2.0f;
	const float statsSpacing = showStats ? style.ItemSpacing.x : 0.0f;
	const float rightAlignedX = ImGui::GetWindowWidth() - style.WindowPadding.x - statsWidth - statsSpacing - cameraButtonWidth;
	const ImVec2 windowPosition = ImGui::GetWindowPos();
	ImGui::SetCursorScreenPos(ImVec2(windowPosition.x + rightAlignedX, windowPosition.y + style.WindowPadding.y));

	ImGui::BeginDisabled(disableInteraction || m_viewportSession == nullptr || m_renderingSettings == nullptr);
	if (ImGui::Button(cameraLabel.c_str()))
	{
		ViewportCameraProperties::OpenPopup();
	}
	if (compact && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Camera properties (%s)", projectionLabel);
	}
	ImGui::EndDisabled();
	if (m_viewportSession != nullptr && m_renderingSettings != nullptr)
	{
		ViewportCameraProperties::BuildPopup(*m_viewportSession, m_renderingSettings->GetState(), disableInteraction);
	}

	if (showStats)
	{
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", statsText);
	}
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

	const bool compactHeader = m_widthPixels < 760.0f;
	const bool showLevel = m_widthPixels >= 480.0f;
	if (showLevel)
	{
		BuildLevelName(compactHeader);
		ImGui::SameLine(0.0f, compactHeader ? 8.0f : 14.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, compactHeader ? 8.0f : 14.0f);
	}
	BuildViewModeCombo(disableInteraction, compactHeader);
	ImGui::SameLine();
	DrawViewportShowMenu(
	    m_consoleVariables,
	    m_viewportSession != nullptr ? m_viewportSession->GetViewMode() : RenderViewMode::Lit,
	    disableInteraction,
	    m_showControlError);
	BuildRightControls(disableInteraction, compactHeader);

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
