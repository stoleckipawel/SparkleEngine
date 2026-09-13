#include "PCH.h"
#include "Panels/ViewportTopPanel.h"

#include "Level/Level.h"
#include "Level/LevelSession.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Style/SparkleUiPalette.h"
#include "Util/UiUtil.h"
#include "Viewport/ViewportCameraProperties.h"
#include "Viewport/EditorViewportSession.h"

#include <imgui.h>

#include <cstring>
#include <cstdio>
#include <string>

class ViewModePresentation final
{
public:
	static UiUtil::EditorIcon GetViewModeIcon(EditorViewportViewMode viewMode) noexcept
	{
		switch (viewMode)
		{
			case EditorViewportViewMode::Lit:
				return UiUtil::EditorIcon::ViewLit;
			case EditorViewportViewMode::ReferencePathTracer:
				return UiUtil::EditorIcon::ViewLit;
			case EditorViewportViewMode::Wireframe:
				return UiUtil::EditorIcon::ViewMode;
			case EditorViewportViewMode::GBufferDiffuse:
				return UiUtil::EditorIcon::ViewDiffuse;
			case EditorViewportViewMode::GBufferNormal:
				return UiUtil::EditorIcon::ViewNormal;
			case EditorViewportViewMode::GBufferRoughness:
				return UiUtil::EditorIcon::ViewRoughness;
			case EditorViewportViewMode::GBufferMetallic:
				return UiUtil::EditorIcon::ViewMetallic;
			case EditorViewportViewMode::GBufferEmissive:
				return UiUtil::EditorIcon::ViewEmissive;
			case EditorViewportViewMode::GBufferAmbientOcclusion:
				return UiUtil::EditorIcon::ViewAmbientOcclusion;
			case EditorViewportViewMode::GBufferSubsurfaceColor:
				return UiUtil::EditorIcon::ViewSubsurfaceColor;
			case EditorViewportViewMode::GBufferSubsurfaceStrength:
				return UiUtil::EditorIcon::ViewSubsurfaceStrength;
			case EditorViewportViewMode::DirectDiffuse:
				return UiUtil::EditorIcon::ViewDirectDiffuse;
			case EditorViewportViewMode::DirectSpecular:
				return UiUtil::EditorIcon::ViewDirectSpecular;
			case EditorViewportViewMode::DirectSubsurface:
				return UiUtil::EditorIcon::ViewDirectSubsurface;
			case EditorViewportViewMode::IndirectDiffuse:
				return UiUtil::EditorIcon::ViewDirectDiffuse;
			case EditorViewportViewMode::IndirectSpecular:
				return UiUtil::EditorIcon::ViewDirectSpecular;
			case EditorViewportViewMode::GpuSceneInstances:
				return UiUtil::EditorIcon::ViewMode;
			case EditorViewportViewMode::Count:
				break;
		}

		return UiUtil::EditorIcon::ViewLit;
	}
};

ViewportTopPanel::ViewportTopPanel(
    LevelSession* levelSession,
    EngineRenderingSettingsSection* renderingSettings,
    EditorViewportSession* viewportSession) noexcept :
    m_renderingSettings(renderingSettings),
    m_viewportSession(viewportSession)
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

const char* ViewportTopPanel::GetViewModeLabel(EditorViewportViewMode viewMode) noexcept
{
	switch (viewMode)
	{
		case EditorViewportViewMode::Lit:
			return "Lit";
		case EditorViewportViewMode::ReferencePathTracer:
			return "Reference Path Tracer (Unavailable)";
		case EditorViewportViewMode::Wireframe:
			return "Wireframe";
		case EditorViewportViewMode::GBufferDiffuse:
			return "GBuffer Diffuse";
		case EditorViewportViewMode::GBufferNormal:
			return "GBuffer Normal";
		case EditorViewportViewMode::GBufferRoughness:
			return "GBuffer Roughness";
		case EditorViewportViewMode::GBufferMetallic:
			return "GBuffer Metallic";
		case EditorViewportViewMode::GBufferEmissive:
			return "GBuffer Emissive";
		case EditorViewportViewMode::GBufferAmbientOcclusion:
			return "GBuffer Ambient Occlusion";
		case EditorViewportViewMode::GBufferSubsurfaceColor:
			return "GBuffer Subsurface Color";
		case EditorViewportViewMode::GBufferSubsurfaceStrength:
			return "GBuffer Subsurface Strength";
		case EditorViewportViewMode::DirectDiffuse:
			return "Direct Diffuse";
		case EditorViewportViewMode::DirectSpecular:
			return "Direct Specular";
		case EditorViewportViewMode::DirectSubsurface:
			return "Direct Subsurface";
		case EditorViewportViewMode::IndirectDiffuse:
			return "Indirect Diffuse";
		case EditorViewportViewMode::IndirectSpecular:
			return "Indirect Specular";
		case EditorViewportViewMode::GpuSceneInstances:
			return "GPU Scene Instances";
		case EditorViewportViewMode::Count:
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

void ViewportTopPanel::DrawViewModeOption(EditorViewportViewMode option, EditorViewportViewMode currentViewMode) noexcept
{
	const bool selected = option == currentViewMode;
	const std::string optionLabel = UiUtil::MakeIconLabel(ViewModePresentation::GetViewModeIcon(option), GetViewModeLabel(option));
	if (ImGui::Selectable(optionLabel.c_str(), selected))
	{
		if (m_viewportSession != nullptr)
		{
			m_viewportSession->SetViewMode(option);
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
	EditorViewportViewMode currentViewMode = m_viewportSession != nullptr ? m_viewportSession->GetViewMode() : EditorViewportViewMode::Lit;
	if (currentViewMode >= EditorViewportViewMode::Count)
	{
		currentViewMode = EditorViewportViewMode::Lit;
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
	const std::string previewLabel =
	    UiUtil::MakeIconLabel(ViewModePresentation::GetViewModeIcon(currentViewMode), GetViewModeLabel(currentViewMode));
	if (ImGui::BeginCombo("##ViewportViewMode", previewLabel.c_str()))
	{
		DrawViewModeOption(EditorViewportViewMode::Lit, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::Wireframe, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GpuSceneInstances, currentViewMode);

		DrawViewModeCategory("GBuffer");
		ImGui::Indent(8.0f);
		DrawViewModeOption(EditorViewportViewMode::GBufferDiffuse, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferNormal, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferRoughness, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferMetallic, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferEmissive, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferAmbientOcclusion, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferSubsurfaceColor, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::GBufferSubsurfaceStrength, currentViewMode);
		ImGui::Unindent(8.0f);

		DrawViewModeCategory("Lighting");
		ImGui::Indent(8.0f);
		DrawViewModeOption(EditorViewportViewMode::DirectDiffuse, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::DirectSpecular, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::DirectSubsurface, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::IndirectDiffuse, currentViewMode);
		DrawViewModeOption(EditorViewportViewMode::IndirectSpecular, currentViewMode);
		ImGui::Unindent(8.0f);

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
