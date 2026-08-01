#include "PCH.h"

#include "D3D12/UI/D3D12ImGuiBackend.h"

#include "Commands/RenderCommandList.h"
#include "Frame/RhiFrameConstants.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"

#include <backends/imgui_impl_dx12.h>

#include <limits>

static const auto g_d3d12ImGuiBackendLogger = Logging::GetOrCreateLogger("RHI.D3D12.ImGui");

D3D12ImGuiBackend::D3D12ImGuiBackend(D3D12RenderHardwareInterface& renderHardwareInterface) noexcept : m_renderHardwareInterface(renderHardwareInterface) {}

void D3D12ImGuiBackend::Initialize()
{
	if (m_imguiContext != nullptr)
	{
		return;
	}

	ImGuiContext* previousContext = ImGui::GetCurrentContext();
	m_imguiContext = previousContext;
	if (m_imguiContext == nullptr)
	{
		m_imguiContext = ImGui::CreateContext();
		m_ownsContext = true;
	}
	ImGui::SetCurrentContext(m_imguiContext);

	ImGui_ImplDX12_InitInfo initInfo = {};
	initInfo.Device = ToD3D12Device(m_renderHardwareInterface.GetDeviceHandle());
	initInfo.CommandQueue = ToD3D12CommandQueue(m_renderHardwareInterface.GetGraphicsQueueHandle());
	initInfo.NumFramesInFlight = static_cast<int>(
	    m_renderHardwareInterface.GetCapabilities().Presentation.MaximumFramesInFlight);
	initInfo.RTVFormat = D3D12TypeConversions::ToDxgiFormat(m_renderHardwareInterface.GetPresentColorFormat());
	initInfo.DSVFormat = D3D12TypeConversions::ToDxgiFormat(m_renderHardwareInterface.GetPresentDepthStencilFormat());
	initInfo.SrvDescriptorHeap = m_renderHardwareInterface.GetD3D12ShaderResourceDescriptorHeap();
	initInfo.SrvDescriptorAllocFn = &D3D12ImGuiBackend::AllocateDescriptor;
	initInfo.SrvDescriptorFreeFn = &D3D12ImGuiBackend::ReleaseDescriptor;
	initInfo.UserData = &m_renderHardwareInterface;

	if (initInfo.Device == nullptr || initInfo.CommandQueue == nullptr || initInfo.SrvDescriptorHeap == nullptr)
	{
		RestoreContext(previousContext);
		Diagnostics::Fatal(g_d3d12ImGuiBackendLogger, __FILE__, __LINE__, "Cannot initialize ImGui with incomplete D3D12 device state.");
	}

	if (!ImGui_ImplDX12_Init(&initInfo))
	{
		RestoreContext(previousContext);
		if (m_ownsContext)
		{
			ImGui::DestroyContext(m_imguiContext);
		}
		m_imguiContext = nullptr;
		m_ownsContext = false;
		Diagnostics::Fatal(g_d3d12ImGuiBackendLogger, __FILE__, __LINE__, "ImGui D3D12 renderer initialization failed.");
	}
	RestoreContext(previousContext);
}

void D3D12ImGuiBackend::BeginFrame() noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	ImGui_ImplDX12_NewFrame();
	RestoreContext(previousContext);
}

std::uint64_t D3D12ImGuiBackend::ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return shaderResourceView.Value;
}

void D3D12ImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	RenderCommandList& commandList = m_renderHardwareInterface.GetGraphicsCommandList(m_renderHardwareInterface.GetCurrentFrameIndex());
	Render(
	    commandList.GetNativeHandle(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::Presentation,
	            .Reason = "Render ImGui draw data through D3D12 backend"}),
	    drawData);
	RestoreContext(previousContext);
}

void D3D12ImGuiBackend::ReleaseTexture(
    ImTextureData& texture) noexcept
{
	ImGuiContext* previousContext = ActivateContext();
	texture.UnusedFrames = (std::numeric_limits<int>::max)();
	texture.WantDestroyNextFrame = true;
	texture.SetStatus(ImTextureStatus_WantDestroy);
	ImGui_ImplDX12_UpdateTexture(&texture);
	RestoreContext(previousContext);
}

void D3D12ImGuiBackend::Render(NativeGraphicsCommandListHandle commandList, ImDrawData* drawData) noexcept
{
	ID3D12GraphicsCommandList* nativeCommandList = ToD3D12GraphicsCommandList(commandList);
	if (nativeCommandList == nullptr || drawData == nullptr)
	{
		return;
	}

	ImGui_ImplDX12_RenderDrawData(drawData, nativeCommandList);
}

void D3D12ImGuiBackend::Shutdown() noexcept
{
	if (m_imguiContext == nullptr)
	{
		return;
	}
	ImGuiContext* previousContext = ActivateContext();
	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_Shutdown();
	RestoreContext(previousContext);
	if (m_ownsContext)
	{
		ImGui::DestroyContext(m_imguiContext);
	}
	m_imguiContext = nullptr;
	m_ownsContext = false;
}

ImGuiContext* D3D12ImGuiBackend::ActivateContext() const noexcept
{
	ImGuiContext* previousContext = ImGui::GetCurrentContext();
	ImGui::SetCurrentContext(m_imguiContext);
	return previousContext;
}

void D3D12ImGuiBackend::RestoreContext(ImGuiContext* context) noexcept
{
	ImGui::SetCurrentContext(context);
}

void D3D12ImGuiBackend::AllocateDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
    D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
{
	auto* renderHardwareInterface = static_cast<D3D12RenderHardwareInterface*>(info->UserData);
	const RhiDescriptorAllocation allocation =
	    renderHardwareInterface->GetDescriptorService().AllocateDescriptor(ERhiDescriptorAllocatorType::ShaderResource);
	*outCpuHandle = ToD3D12CpuDescriptor(allocation.CpuHandle);
	*outGpuHandle = ToD3D12GpuDescriptor(allocation.GpuHandle);
}

void D3D12ImGuiBackend::ReleaseDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	auto* renderHardwareInterface = static_cast<D3D12RenderHardwareInterface*>(info->UserData);
	renderHardwareInterface->GetDescriptorService().ReleaseDescriptor(
	    ERhiDescriptorAllocatorType::ShaderResource,
	    RhiDescriptorAllocation{
	        .CpuHandle = RhiCpuDescriptorHandle{cpuHandle.ptr},
	        .GpuHandle = RhiGpuDescriptorHandle{gpuHandle.ptr}});
}

ID3D12Device* D3D12ImGuiBackend::ToD3D12Device(NativeGraphicsDeviceHandle handle) noexcept
{
	return static_cast<ID3D12Device*>(handle.Value);
}

ID3D12CommandQueue* D3D12ImGuiBackend::ToD3D12CommandQueue(NativeGraphicsQueueHandle handle) noexcept
{
	return static_cast<ID3D12CommandQueue*>(handle.Value);
}

ID3D12GraphicsCommandList* D3D12ImGuiBackend::ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept
{
	return static_cast<ID3D12GraphicsCommandList*>(handle.Value);
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12ImGuiBackend::ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept
{
	D3D12_CPU_DESCRIPTOR_HANDLE nativeHandle{};
	nativeHandle.ptr = handle.Value;
	return nativeHandle;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12ImGuiBackend::ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept
{
	D3D12_GPU_DESCRIPTOR_HANDLE nativeHandle{};
	nativeHandle.ptr = handle.Value;
	return nativeHandle;
}
