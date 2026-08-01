#include "PCH.h"

#include "UI.h"

#include "EditorWorkspaceLayout.h"
#include "Console/EditorConsoleSystem.h"
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
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"
#include "Scene/Model/EditorSceneModel.h"
#include "Scene/Model/EditorSceneModelBuilder.h"
#include "Scene/Transactions/EditorTransactionHistory.h"
#include "Settings/EditorRestartService.h"
#include "Style/SparkleUiTheme.h"
#include "Window/Window.h"

#include <backends/imgui_impl_win32.h>
#include <imgui.h>

#include <memory>
#include <string>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::shared_ptr<spdlog::logger> g_editorLogger = Logging::GetOrCreateLogger("Editor");

void UI::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_isImGuiContextInitialized = true;

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGuiRenderPacketBuilder::ConfigureProducerContext();

	ImGui::StyleColorsDark();
	SparkleUiTheme::ApplyEditorialDarkTheme();
}

bool UI::InitializeWin32Backend()
{
	if (!m_window->GetHWND())
	{
		Diagnostics::Fatal(g_editorLogger, __FILE__, __LINE__, "UI::InitializeWin32Backend: invalid window handle");
		return false;
	}

	ImGui_ImplWin32_Init(m_window->GetHWND());
	m_isWin32BackendInitialized = true;
	return true;
}

void UI::InitializeDefaultPanels()
{
	InitializeCorePanels();
	InitializeViewportPanels();
	InitializeAssetPanels();
	InitializeScenePanels();
}

void UI::InitializeCorePanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelSession, m_window);
	ConfigureMainMenuBarWindowActions();
	m_editorConsoleSystem = std::make_unique<EditorConsoleSystem>();
	m_renderingSettings = std::make_unique<EngineRenderingSettingsSection>();
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

void UI::InitializeViewportPanels()
{
	m_viewportTopPanel = std::make_unique<ViewportTopPanel>(m_levelSession, m_renderingSettings.get());
	m_viewportPanel =
	    std::make_unique<ViewportPanel>(EditorWorkspaceLayout::SceneOutlinerWidth, EditorWorkspaceLayout::SceneInspectorWidth);
}

void UI::InitializeAssetPanels()
{
	m_usedShadersPanel = std::make_unique<UsedShadersPanel>();
	m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	m_usedMeshesPanel = std::make_unique<UsedMeshesPanel>();
	m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
	m_usedMeshesPanel->SetPreviewGeometryProvider(m_meshPreviewProvider);
	m_usedTexturesPanel = std::make_unique<UsedTexturesPanel>();
	m_usedTexturesPanel->SetDiagnosticsProvider(m_textureDiagnosticsProvider);
	m_usedShadersPanel->SetReloadHandler(
	    [this]()
	    {
		    m_shaderReloadRequested = true;
	    });
	m_usedShadersPanel->SetRecookAllHandler(
	    [this]()
	    {
		    m_shaderRecookRequested = true;
	    });
	m_usedShadersPanel->SetRecookHandler(
	    [this](std::string packageId)
	    {
		    if (m_editorConsoleSystem)
		    {
			    m_editorConsoleSystem->SubmitLine("RecompileShaders " + packageId);
		    }
	    });
}

void UI::InitializeScenePanels()
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

void UI::ConfigureMainMenuBarWindowActions()
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
		    m_viewportCaptureRequested = true;
	    });
}

void UI::SubscribeToWindowEvents(Window& window)
{
	auto handle = window.OnWindowMessage.Add(
	    [this](WindowMessageEvent& event)
	    {
		    if (!m_isWin32BackendInitialized)
		    {
			    return;
		    }

		    if (m_editorConsoleSystem != nullptr && ImGui::GetCurrentContext() != nullptr &&
		        m_editorConsoleSystem->HandleShortcut(
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
}

void UI::SetupDPIScaling() noexcept
{
	ImGui_ImplWin32_EnableDpiAwareness();
	const float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	SparkleUiTheme::ConfigureTypography(mainScale);
	ImGuiStyle& style = ImGui::GetStyle();
	style.FontSizeBase = 16.0f * mainScale;
	style.ScaleAllSizes(mainScale);
}
