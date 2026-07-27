#include "PCH.h"

#include "D3D12/UI/D3D12ImGuiBackend.h"

#include "Commands/RenderCommandList.h"
#include "Frame/RhiFrameConstants.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"

#include <backends/imgui_impl_dx12.h>

#include <limits>

D3D12ImGuiBackend::D3D12ImGuiBackend(D3D12RenderHardwareInterface& renderHardware) noexcept : m_renderHardware(&renderHardware) {}

bool D3D12ImGuiBackend::Initialize()
{
	if (m_renderHardware == nullptr || m_imguiContext != nullptr)
	{
		return m_imguiContext != nullptr;
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
	initInfo.Device = ToD3D12Device(m_renderHardware->GetDeviceHandle());
	initInfo.CommandQueue = ToD3D12CommandQueue(m_renderHardware->GetGraphicsQueueHandle());
	initInfo.NumFramesInFlight = static_cast<int>(RhiFrameConstants::FramesInFlight);
	initInfo.RTVFormat = D3D12TypeConversions::ToDxgiFormat(m_renderHardware->GetPresentColorFormat());
	initInfo.DSVFormat = D3D12TypeConversions::ToDxgiFormat(m_renderHardware->GetPresentDepthStencilFormat());
	initInfo.SrvDescriptorHeap = m_renderHardware->GetD3D12ShaderResourceDescriptorHeap();
	initInfo.SrvDescriptorAllocFn = &D3D12ImGuiBackend::AllocateDescriptor;
	initInfo.SrvDescriptorFreeFn = &D3D12ImGuiBackend::ReleaseDescriptor;
	initInfo.UserData = m_renderHardware;

	if (initInfo.Device == nullptr || initInfo.CommandQueue == nullptr || initInfo.SrvDescriptorHeap == nullptr)
	{
		RestoreContext(previousContext);
		return false;
	}

	const bool initialized = ImGui_ImplDX12_Init(&initInfo);
	RestoreContext(previousContext);
	if (!initialized)
	{
		if (m_ownsContext)
		{
			ImGui::DestroyContext(m_imguiContext);
		}
		m_imguiContext = nullptr;
		m_ownsContext = false;
	}
	return initialized;
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
	if (m_renderHardware == nullptr)
	{
		return;
	}

	ImGuiContext* previousContext = ActivateContext();
	RenderCommandList& commandList = m_renderHardware->GetGraphicsCommandList(m_renderHardware->GetCurrentFrameIndex());
	Render(
	    commandList.GetNativeHandle(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::PresentationBridge,
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
	auto* renderHardware = static_cast<D3D12RenderHardwareInterface*>(info->UserData);
	const RhiDescriptorAllocation allocation =
	    renderHardware->GetDescriptorService().AllocateDescriptor(ERhiDescriptorAllocatorType::ShaderResource);
	*outCpuHandle = ToD3D12CpuDescriptor(allocation.CpuHandle);
	*outGpuHandle = ToD3D12GpuDescriptor(allocation.GpuHandle);
}

void D3D12ImGuiBackend::ReleaseDescriptor(
    ImGui_ImplDX12_InitInfo* info,
    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle)
{
	auto* renderHardware = static_cast<D3D12RenderHardwareInterface*>(info->UserData);
	renderHardware->GetDescriptorService().ReleaseDescriptor(
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
