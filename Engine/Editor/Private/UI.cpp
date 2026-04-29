#include "PCH.h"
#include "UI.h"
#include "Window/Window.h"
#include "Config/RenderConfig.h"
#include "Level/LevelManager.h"
#include "Scene/GameScene.h"
#include "Timer.h"

#include "Console/EditorConsoleSystem.h"
#include "Panels/MainMenuBarPanel.h"
#include "Panels/ProfilerPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/UsedShadersPanel.h"
#include "Panels/ViewportPanel.h"
#include "Style/SparkleUiTheme.h"
#include "D3D12/D3D12TypeConversions.h"

#include "Core/Public/Diagnostics/Trace.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

#include <algorithm>
#include <utility>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	constexpr float SceneOutlinerWidth = 320.0f;
	constexpr float SceneInspectorWidth = 560.0f;
	constexpr float MinimumViewportExtent = 64.0f;

	ID3D12Device* ToD3D12Device(NativeGraphicsDeviceHandle handle) noexcept
	{
		return static_cast<ID3D12Device*>(handle.Value);
	}

	ID3D12CommandQueue* ToD3D12CommandQueue(NativeGraphicsQueueHandle handle) noexcept
	{
		return static_cast<ID3D12CommandQueue*>(handle.Value);
	}

	ID3D12DescriptorHeap* ToD3D12DescriptorHeap(NativeDescriptorHeapHandle handle) noexcept
	{
		return static_cast<ID3D12DescriptorHeap*>(handle.Value);
	}

	ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept
	{
		return static_cast<ID3D12GraphicsCommandList*>(handle.Value);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
	{
		D3D12_CPU_DESCRIPTOR_HANDLE nativeHandle{};
		nativeHandle.ptr = handle.Value;
		return nativeHandle;
	}

	D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
	{
		D3D12_GPU_DESCRIPTOR_HANDLE nativeHandle{};
		nativeHandle.ptr = handle.Value;
		return nativeHandle;
	}
}

static void AllocSRV(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
{
	auto* renderHardware = static_cast<RenderHardwareInterface*>(info->UserData);
	RhiCpuDescriptorHandle cpuHandle{};
	RhiGpuDescriptorHandle gpuHandle{};
	renderHardware->AllocateShaderResourceDescriptor(cpuHandle, gpuHandle);
	*out_cpu_handle = ToD3D12CpuDescriptor(cpuHandle);
	*out_gpu_handle = ToD3D12GpuDescriptor(gpuHandle);
}

static void FreeSRV(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
{
	auto* renderHardware = static_cast<RenderHardwareInterface*>(info->UserData);
	static const auto editorLogger = Logging::GetOrCreateLogger("Editor");
	SPDLOG_LOGGER_INFO(
	    editorLogger,
	    "UI::FreeSRV releasing cpu={} gpu={}",
	    static_cast<unsigned long long>(cpu_handle.ptr),
	    static_cast<unsigned long long>(gpu_handle.ptr));
	renderHardware->ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle{cpu_handle.ptr}, RhiGpuDescriptorHandle{gpu_handle.ptr});
	SPDLOG_LOGGER_INFO(editorLogger, "UI::FreeSRV release complete");
}

void UI::HandleWindowMessage(WindowMessageEvent& event) noexcept
{
	if (ProcessWindowMessage(event.hWnd, event.msg, event.wParam, event.lParam))
	{
		event.handled = true;
	}
}

bool UI::ProcessWindowMessage(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
{
	if (!m_isWin32BackendInitialized)
	{
		return false;
	}

	if (m_editorConsoleSystem != nullptr && ImGui::GetCurrentContext() != nullptr &&
	    m_editorConsoleSystem->HandleShortcut(
	        static_cast<std::uint32_t>(msg),
	        static_cast<std::uintptr_t>(wParam),
	        ImGui::GetIO().WantTextInput))
	{
		return true;
	}

	return ImGui_ImplWin32_WndProcHandler(wnd, msg, wParam, lParam);
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

bool UI::WantsGameplayInput() const noexcept
{
	return m_viewportPanel != nullptr && m_viewportPanel->WantsGameplayInput();
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

void UI::SetShaderPackageGenerationProvider(std::function<std::uint64_t()> provider)
{
	m_shaderPackageGenerationProvider = std::move(provider);
	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
	}
	ConfigureMainMenuBarShaderActions();
}

bool UI::ConsumeShaderReloadRequest() noexcept
{
	const bool requested = m_shaderReloadRequested;
	m_shaderReloadRequested = false;
	return requested;
}

void UI::SetShaderRecookStatus(std::string status)
{
	m_shaderRecookStatus = std::move(status);
	m_showShaderRecookStatus = !m_shaderRecookStatus.empty();
	if (m_usedShadersPanel)
	{
		m_usedShadersPanel->SetLastStatus(m_shaderRecookStatus);
	}
}

bool UI::ConsumeShaderRecookRequest() noexcept
{
	const bool requested = m_shaderRecookRequested;
	m_shaderRecookRequested = false;
	return requested;
}

UI::UI(Timer& timer, LevelManager* levelManager, GameScene* gameScene, RenderHardwareInterface& renderHardware, Window& window) :
    m_timer(&timer),
    m_levelManager(levelManager),
    m_gameScene(gameScene),
    m_renderHardware(&renderHardware),
    m_window(&window),
    m_sceneSelection(SceneObjectSelection::None())
{
	InitializeImGuiContext();
	SetupDPIScaling();

	if (!InitializeWin32Backend())
		return;

	if (!InitializeGraphicsBackend())
		return;

	InitializeDefaultPanels();
	SubscribeToWindowEvents(window);
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
	if (m_renderHardware == nullptr)
	{
		Diagnostics::Fail(g_editorLogger, __FILE__, __LINE__, "UI::InitializeGraphicsBackend: missing render hardware interface");
		return false;
	}

	switch (m_renderHardware->GetBackendApi())
	{
		case ERhiBackendApi::D3D12:
			return InitializeNativeGraphicsBackend();
		default:
			Diagnostics::Fail(
			    g_editorLogger,
			    __FILE__,
			    __LINE__,
			    "UI::InitializeGraphicsBackend: editor UI backend is only implemented for D3D12");
			return false;
	}
}

bool UI::InitializeNativeGraphicsBackend()
{
	if (m_renderHardware == nullptr || m_renderHardware->GetBackendApi() != ERhiBackendApi::D3D12)
	{
		Diagnostics::Fail(
		    g_editorLogger,
		    __FILE__,
		    __LINE__,
		    "UI::InitializeNativeGraphicsBackend: invalid render backend for current editor UI initialization");
		return false;
	}

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = ToD3D12Device(m_renderHardware->GetDeviceHandle());
	initInfo.CommandQueue = ToD3D12CommandQueue(m_renderHardware->GetGraphicsQueueHandle());
	initInfo.NumFramesInFlight = static_cast<int>(RenderConfig::FramesInFlight);
	initInfo.RTVFormat = D3D12TypeConversions::ToDxgiFormat(m_renderHardware->GetPresentColorFormat());
	initInfo.DSVFormat = D3D12TypeConversions::ToDxgiFormat(RenderConfig::DepthStencilFormat);
	initInfo.SrvDescriptorHeap = ToD3D12DescriptorHeap(m_renderHardware->GetShaderResourceHeapHandle());
	initInfo.SrvDescriptorAllocFn = &AllocSRV;
	initInfo.SrvDescriptorFreeFn = &FreeSRV;
	initInfo.UserData = m_renderHardware;

	if (initInfo.Device == nullptr || initInfo.CommandQueue == nullptr || initInfo.SrvDescriptorHeap == nullptr)
	{
		Diagnostics::Fail(
		    g_editorLogger,
		    __FILE__,
		    __LINE__,
		    "UI::InitializeNativeGraphicsBackend: missing native device/queue/descriptor-heap");
		return false;
	}

	ImGui_ImplDX12_Init(&initInfo);
	m_isGraphicsBackendInitialized = true;

	return true;
}

void UI::InitializeDefaultPanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelManager, m_window);
	ConfigureMainMenuBarShaderActions();
	m_editorConsoleSystem = std::make_unique<EditorConsoleSystem>();
	m_viewportPanel = std::make_unique<ViewportPanel>(SceneOutlinerWidth, SceneInspectorWidth);
	m_profilerPanel = std::make_unique<ProfilerPanel>();
	m_usedShadersPanel = std::make_unique<UsedShadersPanel>();
	m_usedShadersPanel->SetGenerationProvider(m_shaderPackageGenerationProvider);
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

void UI::ConfigureMainMenuBarShaderActions()
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
	m_mainMenuBar->SetProfilerOpenHandler(
	    [this]()
	    {
		    if (m_profilerPanel)
		    {
			    m_profilerPanel->SetOpen(true);
		    }
	    });
}

void UI::SubscribeToWindowEvents(Window& window)
{
	auto handle = window.OnWindowMessage.Add(
	    [this](WindowMessageEvent& event)
	    {
		    HandleWindowMessage(event);
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

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void UI::Build()
{
	SPARKLE_CPU_SCOPE("Editor.UI.Build");
	const bool disableInteraction = m_levelManager != nullptr && m_levelManager->IsLevelChangeInProgress();
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
	ImGuiIO& io = ImGui::GetIO();
	const float availableCenterHeight = (std::max) (0.0f, io.DisplaySize.y - mainMenuBarHeight);
	const float consoleDockHeight = m_editorConsoleSystem ? m_editorConsoleSystem->GetDockHeight(availableCenterHeight) : 0.0f;
	const float viewportWidth = (std::max) (MinimumViewportExtent, io.DisplaySize.x - outlinerWidth - inspectorWidth);

	if (m_viewportPanel)
	{
		m_viewportPanel->SetTopInset(mainMenuBarHeight);
		m_viewportPanel->SetBottomInset(consoleDockHeight);
		m_viewportPanel->SetSideInsets(outlinerWidth, inspectorWidth);
		m_viewportPanel->BuildUI(disableInteraction);
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

	if (m_profilerPanel)
	{
		m_profilerPanel->BuildUI(disableInteraction);
	}

	if (m_editorConsoleSystem)
	{
		m_editorConsoleSystem->BuildDockedUI(
		    outlinerWidth,
		    mainMenuBarHeight + availableCenterHeight,
		    viewportWidth,
		    availableCenterHeight,
		    disableInteraction);
	}

	BuildShaderRecookStatusWindow(disableInteraction);

#if USE_IMGUI_DEMO_WINDOW
	bool showDemoWindow = true;
	ImGui::ShowDemoWindow(&showDemoWindow);
#endif

	ImGui::Render();
}

void UI::BuildShaderRecookStatusWindow(bool disableInteraction) noexcept
{
	if (!m_showShaderRecookStatus)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(640.0f, 240.0f), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("Shader Recook Status", &m_showShaderRecookStatus))
	{
		ImGui::End();
		return;
	}

	ImGui::BeginDisabled(disableInteraction);
	ImGui::TextWrapped("%s", m_shaderRecookStatus.c_str());
	ImGui::EndDisabled();
	ImGui::End();
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

void UI::Render(NativeGraphicsCommandListHandle commandList) noexcept
{
	SPARKLE_CPU_SCOPE("Editor.UI.Render");
	if (!IsReady())
	{
		return;
	}

	ID3D12GraphicsCommandList* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	if (nativeCommandList == nullptr)
	{
		return;
	}

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), nativeCommandList);
}

UI::~UI() noexcept
{
	SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI begin");

	m_windowMessageHandle.Reset();
	SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI window message handle released");

	if (m_isGraphicsBackendInitialized)
	{
		if (m_renderHardware != nullptr)
		{
			m_renderHardware->WaitForIdle();
		}

		SPDLOG_LOGGER_INFO(
		    g_editorLogger,
		    "UI::~UI graphics backend invalidate begin (context={})",
		    static_cast<const void*>(ImGui::GetCurrentContext()));
		ImGui_ImplDX12_InvalidateDeviceObjects();
		SPDLOG_LOGGER_INFO(g_editorLogger, "UI::~UI graphics backend invalidate complete");
		ImGui_ImplDX12_Shutdown();
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
