#include "PCH.h"
#include "Viewport/ViewportToolbar.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"
#include "Viewport/ViewportCameraProperties.h"
#include "Viewport/EditorViewportSession.h"
#include "Viewport/ViewportShowMenu.h"

#include <imgui.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <string>
#include <string_view>
#include <utility>

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

ViewportToolbar::ViewportToolbar(
    EditorViewportSession& viewportSession,
    const EngineRenderingSettingsState& renderingDefaults,
    const CVarControlExecutor& consoleVariables) noexcept :
    m_viewportSession(viewportSession),
    m_renderingDefaults(renderingDefaults),
    m_consoleVariables(consoleVariables)
{
}

ViewportToolbar::~ViewportToolbar() noexcept = default;

void ViewportToolbar::SetActions(std::unique_ptr<ViewportToolbarActions> actions) noexcept
{
	m_actions = std::move(actions);
}

void ViewportToolbar::SetGeometry(float leftPixels, float topPixels, float widthPixels) noexcept
{
	m_leftPixels = leftPixels;
	m_topPixels = topPixels;
	m_widthPixels = widthPixels;
}

static void DrawViewModeOption(
    EditorViewportSession& viewportSession,
    const ViewModePresentation& option,
    RenderViewMode currentViewMode) noexcept
{
	const bool selected = option.Mode == currentViewMode;
	const UiUtil::MenuCheckState checkState = selected ? UiUtil::MenuCheckState::Checked : UiUtil::MenuCheckState::Unchecked;
	if (UiUtil::DrawMenuItem(option.Label, option.Icon, checkState))
	{
		viewportSession.SetViewMode(option.Mode);
	}

	if (selected)
	{
		ImGui::SetItemDefaultFocus();
	}
}

void ViewportToolbar::DrawLevelName(std::string_view levelName, bool compact) const noexcept
{
	const std::string activeLevelName(levelName);

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

void ViewportToolbar::DrawViewModeSelector(bool disableInteraction, bool compact) noexcept
{
	RenderViewMode currentViewMode = m_viewportSession.GetViewMode();
	if (currentViewMode >= RenderViewMode::Count)
	{
		currentViewMode = RenderViewMode::Lit;
	}

	ImGui::BeginDisabled(disableInteraction);
	const ViewModePresentation& currentPresentation = DescribeViewMode(currentViewMode);
	const std::string previewLabel = UiUtil::MakeIconLabel(currentPresentation.Icon, currentPresentation.Label);
	const float previewWidth =
	    ImGui::CalcTextSize(previewLabel.c_str()).x + ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0f;
	ImGui::SetNextItemWidth((std::clamp) (previewWidth, 100.0f, compact ? 145.0f : 260.0f));
	const UiUtil::MenuStyleScope menuStyle;
	if (ImGui::BeginCombo("##ViewportViewMode", previewLabel.c_str()))
	{
		for (const ViewModePresentation& option : viewModePresentations)
		{
			if (std::string_view(option.Category).empty())
			{
				DrawViewModeOption(m_viewportSession, option, currentViewMode);
			}
		}
		UiUtil::DrawMenuSection("VISUALIZATIONS");
		for (const char* category : {"GBuffer", "Lighting"})
		{
			float childWidth = 0.0f;
			for (const ViewModePresentation& option : viewModePresentations)
			{
				if (std::string_view(option.Category) == category)
				{
					childWidth = (std::max) (childWidth, UiUtil::MeasureMenuRow(option.Label, UiUtil::MenuRowKind::Toggle));
				}
			}
			if (UiUtil::BeginMenu(category, UiUtil::EditorIcon::ViewMode, childWidth))
			{
				for (const ViewModePresentation& option : viewModePresentations)
				{
					if (std::string_view(option.Category) == category)
					{
						DrawViewModeOption(m_viewportSession, option, currentViewMode);
					}
				}
				ImGui::EndMenu();
			}
		}

		ImGui::EndCombo();
	}
	ImGui::EndDisabled();
}

void ViewportToolbar::DrawCameraControls(bool disableInteraction, bool compact) noexcept
{
	const CameraProjectionKind projectionKind = m_viewportSession.GetSettings().ProjectionKind;
	const char* projectionLabel = projectionKind == CameraProjectionKind::Orthographic ? "Orthographic" : "Perspective";
	const std::string cameraText = compact ? UiUtil::GetEditorIconGlyph(UiUtil::EditorIcon::Camera)
	                                       : UiUtil::MakeIconLabel(UiUtil::EditorIcon::Camera, projectionLabel);
	const std::string cameraLabel = cameraText + "##ViewportCameraPropertiesButton";

	ImGui::BeginDisabled(disableInteraction);
	if (ImGui::Button(cameraLabel.c_str()))
	{
		ViewportCameraProperties::OpenPopup();
	}
	if (compact && ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Camera properties (%s)", projectionLabel);
	}
	ImGui::EndDisabled();
	ViewportCameraProperties::BuildPopup(m_viewportSession, m_renderingDefaults, disableInteraction);
}

void ViewportToolbar::DrawActions(float width, bool secondRow, bool disableInteraction) noexcept
{
	if (m_actions == nullptr || width <= 0.0f)
	{
		return;
	}
	const float rightAlignedX = ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - width;
	if (!secondRow)
	{
		ImGui::SameLine(rightAlignedX);
	}
	else
	{
		ImGui::SetCursorPosX((std::max) (ImGui::GetStyle().WindowPadding.x, rightAlignedX));
	}
	m_actions->Draw(disableInteraction);
}

void ViewportToolbar::DrawFrameStats(float actionWidth) const noexcept
{
	const ImGuiIO& io = ImGui::GetIO();
	char statsText[64] = {};
	std::snprintf(statsText, sizeof(statsText), "%.1f FPS  %.2f ms", io.Framerate, io.DeltaTime * 1000.0f);
	const float actionSpacing = actionWidth > 0.0f ? ImGui::GetStyle().ItemSpacing.x : 0.0f;
	const float rightAlignedX =
	    ImGui::GetWindowWidth() - ImGui::GetStyle().WindowPadding.x - ImGui::CalcTextSize(statsText).x - actionWidth - actionSpacing;
	if (rightAlignedX > ImGui::GetCursorPosX() + ImGui::GetStyle().ItemSpacing.x)
	{
		ImGui::SameLine(rightAlignedX);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("%s", statsText);
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Editor frame rate and frame interval");
		}
	}
}

void ViewportToolbar::Draw(std::string_view levelName, bool disableInteraction) noexcept
{
	if (m_widthPixels <= 0.0f)
	{
		m_heightPixels = 0.0f;
		return;
	}

	const ImVec2 windowPadding(10.0f, 4.0f);
	m_heightPixels = ImGui::GetFrameHeight() + (windowPadding.y * 2.0f);
	const bool compactToolbar = m_widthPixels < 760.0f;
	const float actionWidth = m_actions != nullptr ? m_actions->MeasureWidth() : 0.0f;
	const bool actionsOnSecondRow = compactToolbar && actionWidth > 0.0f;
	if (actionsOnSecondRow)
	{
		m_heightPixels += ImGui::GetFrameHeightWithSpacing();
	}

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

	if (!ImGui::Begin("Viewport Toolbar", nullptr, windowFlags))
	{
		ImGui::End();
		ImGui::PopStyleColor(7);
		ImGui::PopStyleVar(2);
		return;
	}

	const bool showLevel = m_widthPixels >= 480.0f;
	if (showLevel)
	{
		DrawLevelName(levelName, compactToolbar);
		ImGui::SameLine(0.0f, compactToolbar ? 8.0f : 14.0f);
		ImGui::AlignTextToFramePadding();
		ImGui::TextDisabled("|");
		ImGui::SameLine(0.0f, compactToolbar ? 8.0f : 14.0f);
	}
	DrawCameraControls(disableInteraction, compactToolbar);
	ImGui::SameLine();
	DrawViewModeSelector(disableInteraction, compactToolbar);
	ImGui::SameLine();
	DrawViewportShowMenu(&m_consoleVariables, disableInteraction, m_showControlError);
	if (!compactToolbar)
	{
		DrawFrameStats(actionWidth);
	}
	DrawActions(actionWidth, actionsOnSecondRow, disableInteraction);

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
