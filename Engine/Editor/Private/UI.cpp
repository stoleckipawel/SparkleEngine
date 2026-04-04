#include "PCH.h"
#include "UI.h"
#include "Window/Window.h"
#include "D3D12Rhi.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12SwapChain.h"
#include "RenderConfig.h"
#include "Level/LevelManager.h"
#include "Scene/GameScene.h"
#include "Timer.h"

#include "Panels/MainMenuBarPanel.h"
#include "Panels/SceneInspectorPanel.h"
#include "Panels/SceneOutlinerPanel.h"
#include "Style/SparkleUiTheme.h"

#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_dx12.h>

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static void AllocSRV(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
    D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
{
	auto* heapManager = static_cast<D3D12DescriptorHeapManager*>(info->UserData);
	heapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, *out_cpu_handle, *out_gpu_handle);
}

static void FreeSRV(ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
{
	auto* heapManager = static_cast<D3D12DescriptorHeapManager*>(info->UserData);
	heapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, cpu_handle, gpu_handle);
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
	return ImGui_ImplWin32_WndProcHandler(wnd, msg, wParam, lParam);
}

UI::UI(
    Timer& timer,
    LevelManager* levelManager,
    GameScene* gameScene,
    D3D12Rhi& rhi,
    Window& window,
    D3D12DescriptorHeapManager& descriptorHeapManager,
    D3D12SwapChain& swapChain) :
    m_timer(&timer),
    m_levelManager(levelManager),
    m_gameScene(gameScene),
    m_rhi(&rhi),
    m_window(&window),
    m_descriptorHeapManager(&descriptorHeapManager),
    m_swapChain(&swapChain)
{
	InitializeImGuiContext();
	SetupDPIScaling();

	if (!InitializeWin32Backend())
		return;

	if (!InitializeD3D12Backend())
		return;

	InitializeDefaultPanels();
	SubscribeToWindowEvents(window);
}

void UI::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

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
	return true;
}

bool UI::InitializeD3D12Backend()
{
	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = m_rhi->GetDevice().Get();
	initInfo.CommandQueue = m_rhi->GetCommandQueue().Get();
	initInfo.NumFramesInFlight = static_cast<int>(RenderConfig::FramesInFlight);
	initInfo.RTVFormat = m_swapChain->GetBackBufferFormat();
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	initInfo.SrvDescriptorHeap = m_descriptorHeapManager->GetHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)->GetRaw();
	initInfo.SrvDescriptorAllocFn = &AllocSRV;
	initInfo.SrvDescriptorFreeFn = &FreeSRV;
	initInfo.UserData = m_descriptorHeapManager;

	if (initInfo.Device == nullptr || initInfo.CommandQueue == nullptr || initInfo.SrvDescriptorHeap == nullptr)
	{
		LOG_FATAL("UI::InitializeD3D12Backend: missing DX12 device/queue/descriptor-heap");
		return false;
	}

	ImGui_ImplDX12_Init(&initInfo);

	return true;
}

void UI::InitializeDefaultPanels()
{
	m_mainMenuBar = std::make_unique<MainMenuBarPanel>(m_levelManager, m_window);
	if (m_gameScene != nullptr)
	{
		m_sceneSelection = SceneObjectSelection::Camera();
		m_sceneOutlinerPanel = std::make_unique<SceneOutlinerPanel>(*m_gameScene, m_sceneSelection);
		m_sceneInspectorPanel = std::make_unique<SceneInspectorPanel>(*m_gameScene, m_sceneSelection);
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
	NewFrame();
	Build();
}

void UI::Render(ID3D12GraphicsCommandList* commandList) noexcept
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}

UI::~UI() noexcept
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
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
