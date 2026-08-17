#include "PCH.h"

#include "UI.h"

#include "EditorWorkspaceLayout.h"
#include "Console/EditorConsoleSystem.h"
#include "Input/InputSystem.h"
#include "Level/LevelSession.h"
#include "Panels/MainMenuBarPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/UsedMeshesPanel.h"
#include "Panels/UsedShadersPanel.h"
#include "Panels/UsedTexturesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ViewportTopPanel.h"
#include "Timer.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <algorithm>

void UI::NewFrame()
{
	if (!IsReady())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void UI::Build()
{
	UpdateSceneModel();
	HandleTransactionShortcuts();
	const bool disableInteraction = m_levelSession != nullptr && m_levelSession->IsLevelChangeInProgress();
	BeginInputRouting(disableInteraction);
	const float mainMenuBarHeight = BuildMainMenuBar();
	BuildSceneOutliner(disableInteraction, mainMenuBarHeight);
	BuildCenterWorkspace(disableInteraction, mainMenuBarHeight);
	BuildSceneInspector(disableInteraction, mainMenuBarHeight);
	BuildUtilityPanels(disableInteraction);

#if USE_IMGUI_DEMO_WINDOW
	bool showDemoWindow = true;
	ImGui::ShowDemoWindow(&showDemoWindow);
#endif

	ImGui::Render();
}

void UI::BeginInputRouting(bool disableInteraction)
{
	if (m_inputSystem == nullptr)
	{
		return;
	}

	const ImGuiIO& io = ImGui::GetIO();
	m_inputSystem->BeginInputRoutingFrame(disableInteraction, io.WantTextInput || io.WantCaptureKeyboard);
}

float UI::BuildMainMenuBar()
{
	if (!m_mainMenuBar)
	{
		return 0.0f;
	}

	m_mainMenuBar->BuildUI();
	return m_mainMenuBar->GetHeight();
}

void UI::BuildSceneOutliner(bool disableInteraction, float mainMenuBarHeight)
{
	if (!m_sceneOutlinerPanel)
	{
		return;
	}

	m_sceneOutlinerPanel->SetTopInset(mainMenuBarHeight);
	m_sceneOutlinerPanel->BuildUI(disableInteraction);
}

void UI::BuildCenterWorkspace(bool disableInteraction, float mainMenuBarHeight)
{
	const ImGuiIO& io = ImGui::GetIO();
	const float outlinerWidth = m_sceneOutlinerPanel ? m_sceneOutlinerPanel->GetWidth() : EditorWorkspaceLayout::SceneOutlinerWidth;
	const float inspectorWidth = m_sceneInspectorPanel ? m_sceneInspectorPanel->GetWidth() : EditorWorkspaceLayout::SceneInspectorWidth;
	const float availableCenterHeight = (std::max) (0.0f, io.DisplaySize.y - mainMenuBarHeight);
	const float viewportWidth =
	    (std::max) (EditorWorkspaceLayout::MinimumViewportExtent, io.DisplaySize.x - outlinerWidth - inspectorWidth);

	float viewportTopPanelHeight = 0.0f;
	if (m_viewportTopPanel)
	{
		m_viewportTopPanel->SetGeometry(outlinerWidth, mainMenuBarHeight, viewportWidth);
		m_viewportTopPanel->BuildUI(disableInteraction);
		viewportTopPanelHeight = m_viewportTopPanel->GetHeight();
	}

	const float availableViewportHeight = (std::max) (0.0f, availableCenterHeight - viewportTopPanelHeight);
	const float consoleDockHeight = m_editorConsoleSystem ? m_editorConsoleSystem->GetDockHeight(availableViewportHeight) : 0.0f;
	BuildViewport(disableInteraction, mainMenuBarHeight + viewportTopPanelHeight, consoleDockHeight, outlinerWidth, inspectorWidth);

	if (m_editorConsoleSystem)
	{
		m_editorConsoleSystem->BuildDockedUI(
		    outlinerWidth,
		    mainMenuBarHeight + availableCenterHeight,
		    viewportWidth,
		    availableViewportHeight,
		    disableInteraction);
	}
}

void UI::BuildViewport(bool disableInteraction, float topInset, float bottomInset, float outlinerWidth, float inspectorWidth)
{
	if (!m_viewportPanel)
	{
		return;
	}

	m_viewportPanel->SetTopInset(topInset);
	m_viewportPanel->SetBottomInset(bottomInset);
	m_viewportPanel->SetSideInsets(outlinerWidth, inspectorWidth);
	m_viewportPanel->BuildUI(disableInteraction);
	RegisterViewportInputRegion();
}

void UI::RegisterViewportInputRegion()
{
	if (!m_viewportPanel || !m_inputSystem)
	{
		return;
	}

	float viewportLeft = 0.0f;
	float viewportTop = 0.0f;
	float viewportRight = 0.0f;
	float viewportBottom = 0.0f;
	if (!m_viewportPanel->GetInputBounds(viewportLeft, viewportTop, viewportRight, viewportBottom))
	{
		return;
	}

	m_inputSystem
	    ->RegisterInputTargetRegion(viewportLeft, viewportTop, viewportRight, viewportBottom, m_viewportPanel->GetTargetInputLayer());
}

void UI::BuildSceneInspector(bool disableInteraction, float mainMenuBarHeight)
{
	if (!m_sceneInspectorPanel)
	{
		return;
	}

	m_sceneInspectorPanel->SetTopInset(mainMenuBarHeight);
	m_sceneInspectorPanel->BuildUI(disableInteraction);
}

void UI::BuildUtilityPanels(bool disableInteraction)
{
	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->BuildUI(disableInteraction);
	}
	if (m_usedMeshesPanel)
	{
		m_usedMeshesPanel->BuildUI(disableInteraction);
	}
	if (m_usedTexturesPanel)
	{
		m_usedTexturesPanel->BuildUI(disableInteraction);
	}
	if (m_settingsPanel)
	{
		m_settingsPanel->BuildUI(disableInteraction);
	}
}
