#include "PCH.h"

#include "D3D12/UI/D3D12ImGuiBackend.h"

#include "Frame/RhiFrameConstants.h"
#include "D3D12/D3D12RenderHardwareInterface.h"
#include "D3D12/D3D12TypeConversions.h"

#include <backends/imgui_impl_dx12.h>

D3D12ImGuiBackend::D3D12ImGuiBackend(D3D12RenderHardwareInterface& renderHardware) noexcept : m_renderHardware(&renderHardware) {}

bool D3D12ImGuiBackend::Initialize()
{
	if (m_renderHardware == nullptr)
	{
		return false;
	}

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
		return false;
	}

	ImGui_ImplDX12_Init(&initInfo);
	return true;
}

void D3D12ImGuiBackend::BeginFrame() noexcept
{
	ImGui_ImplDX12_NewFrame();
}

void D3D12ImGuiBackend::RenderDrawData(ImDrawData* drawData) noexcept
{
	if (m_renderHardware == nullptr)
	{
		return;
	}

	RenderCommandList& commandList = m_renderHardware->GetGraphicsCommandList(m_renderHardware->GetCurrentFrameIndex());
	Render(
	    commandList.GetNativeHandle(
	        RhiNativeInteropRequest{
	            .Consumer = ERhiNativeInteropConsumer::PresentationBridge,
	            .Reason = "Render ImGui draw data through D3D12 backend"}),
	    drawData);
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
	if (m_renderHardware != nullptr)
	{
		m_renderHardware->WaitForIdle();
	}

	ImGui_ImplDX12_InvalidateDeviceObjects();
	ImGui_ImplDX12_Shutdown();
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
