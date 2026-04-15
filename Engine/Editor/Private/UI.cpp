#include "PCH.h"
#include "UI.h"
#include "Window/Window.h"
#include "Config/RenderConfig.h"
#include "Level/LevelManager.h"
#include "Scene/GameScene.h"
#include "Timer.h"

#include "Panels/MainMenuBarPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Panels/ViewportPanel.h"
#include "Style/SparkleUiTheme.h"
#include "D3D12/D3D12TypeConversions.h"

#include <d3d12.h>
#include <dxgi1_6.h>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace
{
	constexpr float SceneOutlinerWidth = 320.0f;
	constexpr float SceneInspectorWidth = 456.0f;

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
	renderHardware->ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle{cpu_handle.ptr}, RhiGpuDescriptorHandle{gpu_handle.ptr});
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

bool UI::InitializeWin32Backend()
{
	if (!m_window->GetHWND())
	{
		LOG_FATAL("UI::InitializeWin32Backend: invalid window handle");
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
		LOG_FATAL("UI::InitializeGraphicsBackend: missing render hardware interface");
		return false;
	}

	switch (m_renderHardware->GetBackendApi())
	{
		case RhiBackendApi::D3D12:
			return InitializeD3D12Backend();
		default:
			LOG_FATAL("UI::InitializeGraphicsBackend: editor UI backend is only implemented for D3D12");
			return false;
	}
}

bool UI::InitializeD3D12Backend()
{
	if (m_renderHardware == nullptr || m_renderHardware->GetBackendApi() != RhiBackendApi::D3D12)
	{
		LOG_FATAL("UI::InitializeD3D12Backend: invalid render backend for D3D12 editor UI initialization");
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
		LOG_FATAL("UI::InitializeD3D12Backend: missing DX12 device/queue/descriptor-heap");
		return false;
	}

	ImGui_ImplDX12_Init(&initInfo);
	m_isGraphicsBackendInitialized = true;

	return true;
}

void UI::InitializeDefaultPanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelManager, m_window);
	m_viewportPanel = std::make_unique<ViewportPanel>(SceneOutlinerWidth, SceneInspectorWidth);
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

	if (m_viewportPanel)
	{
		m_viewportPanel->SetTopInset(mainMenuBarHeight);
		m_viewportPanel->BuildUI(disableInteraction);
	}

	if (m_sceneInspectorPanel)
	{
		m_sceneInspectorPanel->SetTopInset(mainMenuBarHeight);
		m_sceneInspectorPanel->BuildUI(disableInteraction);
	}

#if USE_IMGUI_DEMO_WINDOW
	bool showDemoWindow = true;
	ImGui::ShowDemoWindow(&showDemoWindow);
#endif

	ImGui::Render();
}

void UI::Update()
{
	if (!IsReady())
	{
		return;
	}

	NewFrame();
	Build();
}

void UI::Render(NativeGraphicsCommandListHandle commandList) noexcept
{
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
	if (m_isGraphicsBackendInitialized)
	{
		ImGui_ImplDX12_Shutdown();
	}

	if (m_isWin32BackendInitialized)
	{
		ImGui_ImplWin32_Shutdown();
	}

	if (m_isImGuiContextInitialized)
	{
		ImGui::DestroyContext();
	}
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
