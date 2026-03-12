#include "PCH.h"
#include "UI.h"
#include "Window.h"
#include "D3D12Rhi.h"
#include "D3D12DescriptorHeapManager.h"
#include "D3D12SwapChain.h"
#include "RHIConfig.h"
#include "Timer.h"

#include "Panels/RendererPanel.h"
#include "Sections/StatsOverlay.h"
#include "Sections/ViewMode.h"
#include "Sections/TimeControls.h"

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

UI::UI(Timer& timer, D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager, D3D12SwapChain& swapChain) :
    m_timer(&timer), m_rhi(&rhi), m_window(&window), m_descriptorHeapManager(&descriptorHeapManager), m_swapChain(&swapChain)
{
	InitializeImGuiContext();

	if (!InitializeWin32Backend())
		return;

	if (!InitializeD3D12Backend())
		return;

	SetupDPIScaling();
	InitializeDefaultPanels();
	SubscribeToWindowEvents(window);
}

void UI::InitializeImGuiContext()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();
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
	initInfo.NumFramesInFlight = static_cast<int>(RHISettings::FramesInFlight);
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

	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();

	return true;
}

void UI::InitializeDefaultPanels()
{
	m_rendererPanel = std::make_unique<RendererPanel>();

	if (!m_rendererPanel->HasSection(UIRendererSectionId::Stats))
		m_rendererPanel->SetSection(std::make_unique<StatsOverlay>(*m_timer));

	if (!m_rendererPanel->HasSection(UIRendererSectionId::ViewMode))
		m_rendererPanel->SetSection(std::make_unique<ViewMode>());

	if (!m_rendererPanel->HasSection(UIRendererSectionId::Time))
		m_rendererPanel->SetSection(std::make_unique<TimeControls>(*m_timer));
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

ViewMode::Type UI::GetViewMode() noexcept
{
	if (!m_rendererPanel)
		m_rendererPanel = std::make_unique<RendererPanel>();

	if (!m_rendererPanel->HasSection(UIRendererSectionId::ViewMode))
		m_rendererPanel->SetSection(std::make_unique<ViewMode>());

	const ViewMode& viewMode = static_cast<const ViewMode&>(m_rendererPanel->GetSection(UIRendererSectionId::ViewMode));
	return viewMode.Get();
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
	if (m_rendererPanel)
		m_rendererPanel->BuildUI();

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

void UI::Render() noexcept
{
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_rhi->GetCommandList().Get());
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
	ImGuiStyle& style = ImGui::GetStyle();
	style.FontSizeBase = 16.0f;

	style.ScaleAllSizes(mainScale);
}
