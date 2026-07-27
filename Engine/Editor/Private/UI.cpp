#include "PCH.h"
#include "UI.h"
#include "Window/Window.h"
#include "Input/InputSystem.h"
#include "Level/LevelManager.h"
#include "Timer.h"

#include "Console/EditorConsoleSystem.h"
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
#include "Settings/EditorRestartService.h"
#include "Style/SparkleUiTheme.h"
#include "Scene/Model/EditorSceneModel.h"
#include "Scene/Model/EditorSceneModelBuilder.h"
#include "Scene/Transactions/EditorTransactionManager.h"
#include "Renderer/Public/UI/ImGuiRenderPacketBuilder.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>

#include <algorithm>
#include <utility>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class EditorUiState final
{
  public:
	static constexpr float SceneOutlinerWidth = 320.0f;
	static constexpr float SceneInspectorWidth = 560.0f;
	static constexpr float MinimumViewportExtent = 64.0f;
};

const ViewportRenderRequest& UI::GetViewportRenderRequest() const noexcept
{
	static const ViewportRenderRequest defaultRequest = []
	{
		ViewportRenderRequest request{};
		request.ViewportId = 1;
		request.ViewKind = RenderViewKind::Game;
		request.RequestedOutputs = RenderOutputFlags::SceneColor | RenderOutputFlags::SceneDepth;
		return request;
	}();

	return m_viewportPanel ? m_viewportPanel->GetRenderRequest() : defaultRequest;
}

void UI::SetViewportRenderProducts(const ViewportRenderProducts& products) noexcept
{
	m_viewportGeneration = products.GetGeneration();
	if (m_viewportPanel)
	{
		m_viewportPanel->SetRenderProducts(products);
	}
}

void UI::SetViewportSceneColorTexture(
    EditorTextureHandle texture) noexcept
{
	if (m_viewportPanel)
	{
		m_viewportPanel->SetSceneColorTexture(texture);
	}
}

void UI::SetDiagnosticsProviders(EditorDiagnosticsProviders providers)
{
	m_shaderPackageGenerationProvider = std::move(providers.ShaderPackageGeneration);
	m_meshDiagnosticsProvider = std::move(providers.MeshDiagnostics);
	m_textureDiagnosticsProvider = std::move(providers.TextureDiagnostics);
	m_memoryDiagnosticsProvider = std::move(providers.MemoryDiagnostics);
	m_meshPreviewProvider = std::move(providers.MeshPreview);

	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	}

	if (m_usedMeshesPanel)
	{
		m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
		m_usedMeshesPanel->SetPreviewGeometryProvider(m_meshPreviewProvider);
	}

	if (m_usedTexturesPanel)
	{
		m_usedTexturesPanel->SetDiagnosticsProvider(m_textureDiagnosticsProvider);
	}
	ConfigureMainMenuBarWindowActions();
}

RendererMemoryDiagnosticsSnapshot UI::CaptureMemoryDiagnostics() const
{
	return m_memoryDiagnosticsProvider ? m_memoryDiagnosticsProvider() : RendererMemoryDiagnosticsSnapshot{};
}

bool UI::ConsumeShaderReloadRequest() noexcept
{
	const bool requested = m_shaderReloadRequested;
	m_shaderReloadRequested = false;
	return requested;
}

bool UI::ConsumeShaderRecookRequest() noexcept
{
	const bool requested = m_shaderRecookRequested;
	m_shaderRecookRequested = false;
	return requested;
}

bool UI::ConsumeViewportCaptureRequest() noexcept
{
	const bool requested = m_viewportCaptureRequested;
	m_viewportCaptureRequested = false;
	return requested;
}

UI::UI(EditorHostServices hostServices) :
	m_timer(&hostServices.RuntimeTimer),
	m_levelManager(hostServices.Levels),
	m_window(&hostServices.HostWindow),
	m_inputSystem(&hostServices.Input),
	m_sceneSelection(SceneObjectSelection::None())
{
	m_sceneModelBuilder = std::make_unique<EditorSceneModelBuilder>(EditorSceneSource{
	    .AcquireReadView = std::move(hostServices.AcquireWorldReadView),
	    .ReadChanges = std::move(hostServices.ReadWorldChanges),
	    .AcknowledgeChanges = std::move(hostServices.AcknowledgeWorldChanges),
	    .WorldGeneration = std::move(hostServices.WorldGeneration),
	    .MaterialVariants = std::move(hostServices.MaterialVariants)});
	m_transactions = std::make_unique<EditorTransactionManager>(std::move(hostServices.SubmitWorldEdit));
	m_renderPacketBuilder = std::make_unique<ImGuiRenderPacketBuilder>();
	InitializeImGuiContext();
	SetupDPIScaling();

	if (!InitializeWin32Backend())
	{
		return;
	}

	InitializeDefaultPanels();
	if (m_renderingSettings)
	{
		m_renderingSettings->SetCommitHandler(std::move(hostServices.SubmitRenderingSettings));
	}
	SubscribeToWindowEvents(hostServices.HostWindow);
}

void UI::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_isImGuiContextInitialized = true;

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.Fonts->SetTexID(static_cast<ImTextureID>(EditorTextureHandle::FontAtlas().Pack()));

	ImGui::StyleColorsDark();
	SparkleUiTheme::ApplyEditorialDarkTheme();
}

static std::shared_ptr<spdlog::logger> g_editorLogger = Logging::GetOrCreateLogger("Editor");

bool UI::InitializeWin32Backend()
{
	if (!m_window->GetHWND())
	{
		Diagnostics::Fail(g_editorLogger, __FILE__, __LINE__, "UI::InitializeWin32Backend: invalid window handle");
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
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelManager, m_window);
	ConfigureMainMenuBarWindowActions();
	m_editorConsoleSystem = std::make_unique<EditorConsoleSystem>();
	m_renderingSettings = std::make_unique<EngineRenderingSettingsSection>();
	m_restartService = std::make_unique<EditorRestartService>();
	m_settingsPanel = std::make_unique<SettingsPanel>();
	m_settingsPanel->SetRenderingSettings(m_renderingSettings.get());
	m_settingsPanel->SetRestartHandler(
	    [this]()
	    {
		    if (m_window == nullptr || m_restartService == nullptr)
		    {
			    return;
		    }

		    (void) m_restartService->Restart(*m_window);
	    });
}

void UI::InitializeViewportPanels()
{
	m_viewportTopPanel = std::make_unique<ViewportTopPanel>(m_levelManager, m_renderingSettings.get());
	m_viewportPanel = std::make_unique<ViewportPanel>(
	    EditorUiState::SceneOutlinerWidth,
	    EditorUiState::SceneInspectorWidth);
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

	m_sceneOutlinerPanel = std::make_unique<SceneOutlinerPanel>(m_sceneSelection, *m_transactions, EditorUiState::SceneOutlinerWidth);
	m_sceneInspectorPanel = std::make_unique<SceneInspectorPanel>(m_sceneSelection, *m_transactions, EditorUiState::SceneInspectorWidth);
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

void UI::NewFrame()
{
	if (!IsReady())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));
	io.DisplaySize = ImVec2(m_window->GetWidth(), m_window->GetHeight());

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void UI::Build()
{
	UpdateSceneModel();
	HandleTransactionShortcuts();
	const bool disableInteraction = m_levelManager != nullptr && m_levelManager->IsLevelChangeInProgress();
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
	const float outlinerWidth = m_sceneOutlinerPanel ? m_sceneOutlinerPanel->GetWidth() : EditorUiState::SceneOutlinerWidth;
	const float inspectorWidth = m_sceneInspectorPanel ? m_sceneInspectorPanel->GetWidth() : EditorUiState::SceneInspectorWidth;
	const float availableCenterHeight = (std::max) (0.0f, io.DisplaySize.y - mainMenuBarHeight);
	const float viewportWidth = (std::max) (EditorUiState::MinimumViewportExtent, io.DisplaySize.x - outlinerWidth - inspectorWidth);

	float viewportTopPanelHeight = 0.0f;
	if (m_viewportTopPanel)
	{
		m_viewportTopPanel->SetGeometry(outlinerWidth, mainMenuBarHeight, viewportWidth);
		m_viewportTopPanel->BuildUI(disableInteraction);
		viewportTopPanelHeight = m_viewportTopPanel->GetHeight();
	}

	const float availableViewportHeight = (std::max) (0.0f, availableCenterHeight - viewportTopPanelHeight);
	const float consoleDockHeight = m_editorConsoleSystem ? m_editorConsoleSystem->GetDockHeight(availableViewportHeight) : 0.0f;
	BuildViewport(
	    disableInteraction,
	    mainMenuBarHeight + viewportTopPanelHeight,
	    consoleDockHeight,
	    outlinerWidth,
	    inspectorWidth);

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

UiRenderPacket UI::ConsumeRenderPacket()
{
	return std::move(m_renderPacket);
}

void UI::BuildViewport(
    bool disableInteraction,
    float topInset,
    float bottomInset,
    float outlinerWidth,
    float inspectorWidth)
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

	m_inputSystem->RegisterInputTargetRegion(
	    viewportLeft,
	    viewportTop,
	    viewportRight,
	    viewportBottom,
	    m_viewportPanel->GetTargetInputLayer());
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

void UI::UpdateSceneModel()
{
	if (!m_sceneModelBuilder)
	{
		return;
	}

	const std::uint64_t previousWorldGeneration = m_sceneModel ? m_sceneModel->GetWorldGeneration() : 0;
	m_sceneModel = m_sceneModelBuilder->Update();
	if (!m_sceneModel)
	{
		return;
	}

	if (m_transactions)
	{
		m_transactions->InvalidateForWorldGeneration(m_sceneModel->GetWorldGeneration());
	}

	if (previousWorldGeneration != 0 && previousWorldGeneration != m_sceneModel->GetWorldGeneration())
	{
		m_sceneSelection = SceneObjectSelection::None();
	}

	if (!m_sceneSelection.IsNone() && !m_sceneModel->Contains(m_sceneSelection))
	{
		m_sceneSelection = SceneObjectSelection::None();
	}

	if (m_sceneOutlinerPanel)
	{
		m_sceneOutlinerPanel->SetModel(m_sceneModel);
	}

	if (m_sceneInspectorPanel)
	{
		m_sceneInspectorPanel->SetModel(m_sceneModel);
	}
}

void UI::HandleTransactionShortcuts()
{
	if (!m_sceneModel || !m_transactions || ImGui::GetIO().WantTextInput)
	{
		return;
	}

	if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Z))
	{
		(void) m_transactions->Undo(m_sceneModel->GetWorldGeneration());
	}
	else if (ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_Y))
	{
		(void) m_transactions->Redo(m_sceneModel->GetWorldGeneration());
	}
}
void UI::Update()
{
	if (!IsReady())
	{
		return;
	}

	NewFrame();
	Build();
	m_renderPacket = m_renderPacketBuilder->Build(
	    *ImGui::GetDrawData(),
	    UiPresentationMode::EditorViewport,
	    m_viewportGeneration);
}

UI::~UI() noexcept
{

	m_windowMessageHandle.Reset();

	if (m_isWin32BackendInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		m_isWin32BackendInitialized = false;
	}

	if (m_isImGuiContextInitialized)
	{
		ImGui::DestroyContext();
		m_isImGuiContextInitialized = false;
	}

}

bool UI::IsReady() const noexcept
{
	return m_isImGuiContextInitialized && m_isWin32BackendInitialized;
}

void UI::SetupDPIScaling() noexcept
{
	ImGui_ImplWin32_EnableDpiAwareness();
	float mainScale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	SparkleUiTheme::ConfigureTypography(mainScale);
	ImGuiStyle& style = ImGui::GetStyle();
	style.FontSizeBase = 16.0f * mainScale;

	style.ScaleAllSizes(mainScale);
}
