#include "PCH.h"
#include "UI.h"
#include "Window/Window.h"
#include "Input/InputSystem.h"
#include "Level/LevelManager.h"
#include "Scene/GameScene.h"
#include "Timer.h"

#include "Console/EditorConsoleSystem.h"
#include "Panels/MainMenuBarPanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/SettingsPanel.h"
#include "Panels/UsedMeshesPanel.h"
#include "Panels/UsedShadersPanel.h"
#include "Panels/UsedTexturesPanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/ViewportTopPanel.h"
#include "Settings/EditorRenderingSettings.h"
#include "Style/SparkleUiTheme.h"

#include "Core/Public/Diagnostics/Trace.h"
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "Scene/Meshes/Mesh.h"
#include "Scene/Meshes/MeshComponent.h"
#include "Scene/Meshes/MeshData.h"
#include "Scene/Meshes/SceneMeshes.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>

#include <algorithm>
#include <utility>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	constexpr float SceneOutlinerWidth = 320.0f;
	constexpr float SceneInspectorWidth = 560.0f;
	constexpr float MinimumViewportExtent = 64.0f;
}

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
	if (m_viewportPanel)
	{
		m_viewportPanel->SetRenderProducts(products);
	}
}

void UI::SetViewportSceneColorTextureId(std::uint64_t textureId) noexcept
{
	if (m_viewportPanel)
	{
		m_viewportPanel->SetSceneColorTextureId(textureId);
	}
}

void UI::SetDiagnosticsProviders(EditorDiagnosticsProviders providers)
{
	m_shaderPackageGenerationProvider = std::move(providers.ShaderPackageGeneration);
	m_meshDiagnosticsProvider = std::move(providers.MeshDiagnostics);
	m_textureDiagnosticsProvider = std::move(providers.TextureDiagnostics);
	m_memoryDiagnosticsProvider = std::move(providers.MemoryDiagnostics);
	m_rendererSmokeDiagnosticsProvider = std::move(providers.RendererSmokeDiagnostics);

	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	}

	if (m_usedMeshesPanel)
	{
		m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
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

UI::UI(EditorHostServices hostServices) :
	m_timer(&hostServices.RuntimeTimer),
	m_levelManager(hostServices.Levels),
	m_gameScene(hostServices.Scene),
	m_imguiRenderer(&hostServices.ImGuiRenderer),
	m_window(&hostServices.HostWindow),
	m_inputSystem(&hostServices.Input),
	m_sceneSelection(SceneObjectSelection::None())
{
	InitializeImGuiContext();
	SetupDPIScaling();

	if (!InitializeWin32Backend())
		return;

	if (!InitializeGraphicsBackend())
		return;

	InitializeDefaultPanels();
	SubscribeToWindowEvents(hostServices.HostWindow);
}

void UI::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	m_isImGuiContextInitialized = true;

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

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

bool UI::InitializeGraphicsBackend()
{
	if (m_imguiRenderer == nullptr)
	{
		Diagnostics::Fail(g_editorLogger, __FILE__, __LINE__, "UI::InitializeGraphicsBackend: missing ImGui renderer");
		return false;
	}

	m_isGraphicsBackendInitialized = m_imguiRenderer->Initialize();
	if (!m_isGraphicsBackendInitialized)
	{
		Diagnostics::Fail(
		    g_editorLogger,
		    __FILE__,
		    __LINE__,
		    "UI::InitializeGraphicsBackend: editor UI backend is not implemented for the active RHI backend");
	}

	return m_isGraphicsBackendInitialized;
}

void UI::InitializeDefaultPanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelManager, m_window);
	ConfigureMainMenuBarWindowActions();
	m_editorConsoleSystem = std::make_unique<EditorConsoleSystem>();
	m_viewportTopPanel = std::make_unique<ViewportTopPanel>(m_levelManager);
	m_viewportPanel = std::make_unique<ViewportPanel>(SceneOutlinerWidth, SceneInspectorWidth);
	m_profilerPanel = std::make_unique<ProfilerPanel>();
	m_renderingSettings = std::make_unique<EditorRenderingSettingsSection>();
	m_settingsPanel = std::make_unique<SettingsPanel>();
	m_settingsPanel->SetRenderingSettings(m_renderingSettings.get());
	m_usedShadersPanel = std::make_unique<UsedShadersPanel>();
	m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	m_usedMeshesPanel = std::make_unique<UsedMeshesPanel>();
	m_usedMeshesPanel->SetDiagnosticsProvider(m_meshDiagnosticsProvider);
	m_usedMeshesPanel->SetPreviewGeometryProvider(
	    [this](std::uintptr_t meshRuntimeId)
	    {
		    return BuildMeshPreviewGeometry(meshRuntimeId);
	    });
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
	if (m_window != nullptr && m_viewportPanel)
	{
		const RenderViewportExtent initialExtent{
		    static_cast<std::uint32_t>(
		        (std::max) (1.0f, static_cast<float>(m_window->GetWidth()) - SceneOutlinerWidth - SceneInspectorWidth)),
		    (std::max) (1u, m_window->GetHeight())};
		m_viewportPanel->SetRequestedExtent(initialExtent);
	}

	if (m_gameScene != nullptr)
	{
		m_sceneSelection = SceneObjectSelection::Camera();
		m_sceneOutlinerPanel = std::make_unique<SceneOutlinerPanel>(*m_gameScene, m_sceneSelection, SceneOutlinerWidth);
		m_sceneInspectorPanel = std::make_unique<SceneInspectorPanel>(*m_gameScene, m_sceneSelection, SceneInspectorWidth);
	}
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
	m_mainMenuBar->SetProfilerOpenHandler(
	    [this]()
	    {
		    if (m_profilerPanel)
		    {
			    m_profilerPanel->SetOpen(true);
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
}

MeshPreviewGeometry UI::BuildMeshPreviewGeometry(std::uintptr_t meshRuntimeId) const
{
	MeshPreviewGeometry geometry;
	if (m_gameScene == nullptr || meshRuntimeId == 0)
	{
		return geometry;
	}

	const SceneMeshes& sceneMeshes = m_gameScene->GetMeshes();
	for (std::size_t meshIndex = 0; meshIndex < sceneMeshes.GetMeshCount(); ++meshIndex)
	{
		const MeshComponent* meshComponent = sceneMeshes.GetMeshComponent(meshIndex);
		if (meshComponent == nullptr)
		{
			continue;
		}

		const Mesh* mesh = meshComponent->GetMesh();
		if (mesh == nullptr || reinterpret_cast<std::uintptr_t>(mesh) != meshRuntimeId)
		{
			continue;
		}

		const MeshData& meshData = mesh->GetMeshData();
		geometry.Vertices.reserve(meshData.vertices.size());
		for (const VertexData& vertex : meshData.vertices)
		{
			geometry.Vertices.push_back(MeshPreviewVertex{vertex.position.x, vertex.position.y, vertex.position.z});
		}
		geometry.Indices.assign(meshData.indices.begin(), meshData.indices.end());
		return geometry;
	}

	return geometry;
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
	SPARKLE_CPU_SCOPE("Editor.UI.NewFrame");
	if (!IsReady())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	io.DeltaTime = static_cast<float>(m_timer->GetDelta(TimeDomain::Unscaled, TimeUnit::Seconds));
	io.DisplaySize = ImVec2(m_window->GetWidth(), m_window->GetHeight());

	m_imguiRenderer->BeginFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void UI::Build()
{
	SPARKLE_CPU_SCOPE("Editor.UI.Build");
	const bool disableInteraction = m_levelManager != nullptr && m_levelManager->IsLevelChangeInProgress();
	ImGuiIO& io = ImGui::GetIO();
	if (m_inputSystem != nullptr)
	{
		m_inputSystem->BeginInputRoutingFrame(disableInteraction, io.WantTextInput || io.WantCaptureKeyboard);
	}
	float mainMenuBarHeight = 0.0f;
	if (m_mainMenuBar)
	{
		m_mainMenuBar->BuildUI();
		mainMenuBarHeight = m_mainMenuBar->GetHeight();
	}

	if (m_sceneOutlinerPanel)
	{
		m_sceneOutlinerPanel->SetTopInset(mainMenuBarHeight);
		m_sceneOutlinerPanel->BuildUI(disableInteraction);
	}

	const float outlinerWidth = m_sceneOutlinerPanel ? m_sceneOutlinerPanel->GetWidth() : SceneOutlinerWidth;
	const float inspectorWidth = m_sceneInspectorPanel ? m_sceneInspectorPanel->GetWidth() : SceneInspectorWidth;
	const float availableCenterHeight = (std::max) (0.0f, io.DisplaySize.y - mainMenuBarHeight);
	const float viewportWidth = (std::max) (MinimumViewportExtent, io.DisplaySize.x - outlinerWidth - inspectorWidth);
	float viewportTopPanelHeight = 0.0f;
	if (m_viewportTopPanel)
	{
		m_viewportTopPanel->SetGeometry(outlinerWidth, mainMenuBarHeight, viewportWidth);
		m_viewportTopPanel->BuildUI(disableInteraction);
		viewportTopPanelHeight = m_viewportTopPanel->GetHeight();
	}

	const float availableViewportColumnHeight = (std::max) (0.0f, availableCenterHeight - viewportTopPanelHeight);
	const float consoleDockHeight = m_editorConsoleSystem ? m_editorConsoleSystem->GetDockHeight(availableViewportColumnHeight) : 0.0f;

	if (m_viewportPanel)
	{
		m_viewportPanel->SetTopInset(mainMenuBarHeight + viewportTopPanelHeight);
		m_viewportPanel->SetBottomInset(consoleDockHeight);
		m_viewportPanel->SetSideInsets(outlinerWidth, inspectorWidth);
		m_viewportPanel->SetRendererSmokeDiagnostics(
		    m_rendererSmokeDiagnosticsProvider ? m_rendererSmokeDiagnosticsProvider() : RendererSmokeDiagnosticsSnapshot{});
		m_viewportPanel->BuildUI(disableInteraction);
		if (m_inputSystem != nullptr)
		{
			float viewportLeft = 0.0f;
			float viewportTop = 0.0f;
			float viewportRight = 0.0f;
			float viewportBottom = 0.0f;
			if (m_viewportPanel->GetInputBounds(viewportLeft, viewportTop, viewportRight, viewportBottom))
			{
				m_inputSystem->RegisterInputTargetRegion(
				    viewportLeft,
				    viewportTop,
				    viewportRight,
				    viewportBottom,
				    m_viewportPanel->GetTargetInputLayer());
			}
		}
	}

	if (m_sceneInspectorPanel)
	{
		m_sceneInspectorPanel->SetTopInset(mainMenuBarHeight);
		m_sceneInspectorPanel->BuildUI(disableInteraction);
	}

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

	if (m_profilerPanel)
	{
		m_profilerPanel->BuildUI(disableInteraction);
	}

	if (m_settingsPanel)
	{
		m_settingsPanel->BuildUI(disableInteraction);
	}

	if (m_editorConsoleSystem)
	{
		m_editorConsoleSystem->BuildDockedUI(
		    outlinerWidth,
		    mainMenuBarHeight + availableCenterHeight,
		    viewportWidth,
		    availableViewportColumnHeight,
		    disableInteraction);
	}

#if USE_IMGUI_DEMO_WINDOW
	bool showDemoWindow = true;
	ImGui::ShowDemoWindow(&showDemoWindow);
#endif

	ImGui::Render();
}
void UI::Update()
{
	SPARKLE_CPU_SCOPE("Editor.UI.Update");
	if (!IsReady())
	{
		return;
	}

	NewFrame();
	Build();
}

void UI::Render() noexcept
{
	SPARKLE_CPU_SCOPE("Editor.UI.Render");
	if (!IsReady() || m_imguiRenderer == nullptr)
	{
		return;
	}

	m_imguiRenderer->RenderDrawData(ImGui::GetDrawData());
}

UI::~UI() noexcept
{
	SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI begin");

	m_windowMessageHandle.Reset();
	SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI window message handle released");

	if (m_isGraphicsBackendInitialized)
	{
		SPDLOG_LOGGER_INFO(
		    g_editorLogger,
		    "UI::~UI graphics backend invalidate begin (context={})",
		    static_cast<const void*>(ImGui::GetCurrentContext()));
		m_imguiRenderer->Shutdown();
		SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI graphics backend invalidate complete");
		m_isGraphicsBackendInitialized = false;
		SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI graphics backend shutdown complete");
	}

	if (m_isWin32BackendInitialized)
	{
		ImGui_ImplWin32_Shutdown();
		m_isWin32BackendInitialized = false;
		SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI Win32 backend shutdown complete");
	}

	if (m_isImGuiContextInitialized)
	{
		ImGui::DestroyContext();
		m_isImGuiContextInitialized = false;
		SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI ImGui context destroyed");
	}

	SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI end");
}

bool UI::IsReady() const noexcept
{
	return m_isImGuiContextInitialized && m_isWin32BackendInitialized && m_isGraphicsBackendInitialized;
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
