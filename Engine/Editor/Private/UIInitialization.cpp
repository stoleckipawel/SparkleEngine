#include "PCH.h"

#include "UIImplementation.h"

#include "EditorWorkspaceLayout.h"
#include "Console/EditorConsoleSystem.h"
#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Paths/ProductUserStatePaths.h"
#include "Input/InputSystem.h"
#include "Panels/MainMenuBarPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/UsedMeshesPanel.h"
#include "Panels/UsedShadersPanel.h"
#include "Panels/UsedTexturesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ViewportTopPanel.h"
#include "Settings/EngineRenderingSettingsController.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"
#include "Scene/Model/EditorSceneModel.h"
#include "Scene/Model/EditorSceneModelBuilder.h"
#include "Settings/EditorRestartService.h"
#include "Style/SparkleUiTheme.h"
#include "Viewport/EditorViewportSession.h"
#include "Window/Window.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <memory>
#include <string>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

SPARKLE_DEFINE_LOG_CATEGORY_STATIC(g_editorLogger, "Editor");

void UI::Implementation::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_isImGuiContextInitialized = true;

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	const Filesystem::ProductUserStatePaths& userState = Filesystem::GetProductUserStatePaths();
	m_imguiIniPath = (userState.SettingsRoot / "EditorLayout.ini").string();
	io.IniFilename = m_imguiIniPath.c_str();
	ImGuiRenderPacketBuilder::ConfigureProducerContext();

	SparkleUiTheme::ConfigureTypography();
}

bool UI::Implementation::InitializeWin32Backend()
{
	if (!m_window->GetHWND())
	{
		Diagnostics::Fatal(g_editorLogger, __FILE__, __LINE__, "UI::Implementation::InitializeWin32Backend: invalid window handle");
		return false;
	}

	ImGui_ImplWin32_Init(m_window->GetHWND());
	m_isWin32BackendInitialized = true;
	return true;
}

void UI::Implementation::InitializeDefaultPanels()
{
	InitializeCorePanels();
	InitializeViewportPanels();
	InitializeAssetPanels();
	InitializeScenePanels();
}

void UI::Implementation::InitializeCorePanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelSession, m_window);
	ConfigureMainMenuBarWindowActions();
	m_restartService = std::make_unique<EditorRestartService>();
	m_settingsPanel = std::make_unique<SettingsPanel>();
	m_settingsPanel->SetRenderingSettings(m_renderingSettings.get());
	m_settingsPanel->SetRestartHandler(
	    [this]()
	    {
		    if (m_window != nullptr && m_restartService != nullptr)
		    {
			    (void) m_restartService->Restart(*m_window);
		    }
	    });
}

void UI::Implementation::InitializeViewportPanels()
{
	m_viewportSession = std::make_unique<EditorViewportSession>();
	m_viewportTopPanel =
	    std::make_unique<ViewportTopPanel>(m_levelSession, m_renderingSettings.get(), m_viewportSession.get(), &m_consoleVariables);
	m_viewportPanel =
	    std::make_unique<ViewportPanel>(EditorWorkspaceLayout::SceneOutlinerWidth, EditorWorkspaceLayout::SceneInspectorWidth);
	m_viewportPanel->SetExposureOverrides(m_viewportSession->GetSettings().Exposure);
}

void UI::Implementation::InitializeAssetPanels()
{
	m_usedShadersPanel = std::make_unique<UsedShadersPanel>();
	m_usedShadersPanel->SetGenerationProvider(m_shaderGenerationProvider);
	m_usedMeshesPanel = std::make_unique<UsedMeshesPanel>();
	m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
	m_usedMeshesPanel->SetPreviewGeometryProvider(m_meshPreviewProvider);
	m_usedTexturesPanel = std::make_unique<UsedTexturesPanel>();
	m_usedTexturesPanel->SetDiagnosticsProvider(m_textureDiagnosticsProvider);
	m_usedShadersPanel->SetReloadHandler([this]() { m_shaderReloadRequested = true; });
	m_usedShadersPanel->SetRecookAllHandler([this]() { m_shaderRecookRequested = true; });
	m_usedShadersPanel->SetRecookHandler(
	    [this](const std::string& shaderId)
	    {
		    if (m_editorConsoleSystem)
		    {
			    m_editorConsoleSystem->SubmitLine("RecompileShaders Shader " + shaderId);
		    }
	    });
}

void UI::Implementation::InitializeScenePanels()
{
	m_sceneModel = m_sceneModelBuilder->Update();
	if (m_sceneModel && !m_sceneModel->GetCameras().empty())
	{
		m_sceneSelection = SceneObjectSelection::Camera(m_sceneModel->GetCameras().front().Entity);
	}

	m_sceneOutlinerPanel =
	    std::make_unique<SceneOutlinerPanel>(m_sceneSelection, *m_transactionHistory, EditorWorkspaceLayout::SceneOutlinerWidth);
	m_sceneInspectorPanel =
	    std::make_unique<SceneInspectorPanel>(m_sceneSelection, *m_transactionHistory, EditorWorkspaceLayout::SceneInspectorWidth);
}

void UI::Implementation::ConfigureMainMenuBarWindowActions()
{
	if (!m_mainMenuBar)
	{
		return;
	}

	m_mainMenuBar->SetShaderToolsOpenHandler(
	    [this]()
	    {
		    if (m_usedShadersPanel)
		    {
			    m_usedShadersPanel->SetOpen(true);
		    }
	    });
	m_mainMenuBar->SetTextureToolsOpenHandler(
	    [this]()
	    {
		    if (m_usedTexturesPanel)
		    {
			    m_usedTexturesPanel->SetOpen(true);
		    }
	    });
	m_mainMenuBar->SetMeshToolsOpenHandler(
	    [this]()
	    {
		    if (m_usedMeshesPanel)
		    {
			    m_usedMeshesPanel->SetOpen(true);
		    }
	    });
	m_mainMenuBar->SetSettingsOpenHandler(
	    [this]()
	    {
		    if (m_settingsPanel)
		    {
			    m_settingsPanel->SetOpen(true);
		    }
	    });
	m_mainMenuBar->SetViewportCaptureHandler(
	    [this]()
	    {
		    if (m_viewportPanel)
		    {
			    m_viewportPanel->RequestOutputAction(ViewportOutputAction::CapturePresentation);
		    }
	    });
	m_mainMenuBar->SetConsoleOpenHandler(
	    [this]()
	    {
		    if (m_editorConsoleSystem)
		    {
			    m_editorConsoleSystem->OpenConsole();
		    }
	    });
}

void UI::Implementation::SubscribeToWindowEvents(Window& window)
{
	auto handle = window.OnWindowMessage.Add(
	    [this](WindowMessageEvent& event)
	    {
		    if (!m_isWin32BackendInitialized)
		    {
			    return;
		    }

		    if (m_editorConsoleSystem != nullptr && ImGui::GetCurrentContext() != nullptr && (event.lParam & (LPARAM{1} << 30)) == 0
		        && m_editorConsoleSystem->HandleShortcut(
		            static_cast<std::uint32_t>(event.msg),
		            static_cast<std::uintptr_t>(event.wParam),
		            ImGui::GetIO().WantTextInput))
		    {
			    event.handled = true;
			    return;
		    }

		    if (ImGui_ImplWin32_WndProcHandler(event.hWnd, event.msg, event.wParam, event.lParam) != 0)
		    {
			    event.handled = true;
		    }
	    });
	m_windowMessageHandle = ScopedEventHandle(window.OnWindowMessage, handle);

	auto dpiScaleHandle = window.OnDpiScaleChanged.Add([this](float dpiScale) { ApplyDpiScale(dpiScale); });
	m_windowDpiScaleHandle = ScopedEventHandle(window.OnDpiScaleChanged, dpiScaleHandle);
}

void UI::Implementation::ApplyDpiScale(float dpiScale) noexcept
{
	ImGuiStyle& style = ImGui::GetStyle();
	style = ImGuiStyle{};
	ImGui::StyleColorsDark(&style);
	SparkleUiTheme::ApplyEditorialDarkTheme();
	style.FontScaleDpi = dpiScale;
	style.ScaleAllSizes(dpiScale);
}
