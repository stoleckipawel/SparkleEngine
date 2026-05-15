#pragma once

#include "Device/RenderHardwareInterface.h"

#include <d3d12.h>

struct ImDrawData;
struct ImGui_ImplDX12_InitInfo;

class D3D12RenderHardwareInterface;

class D3D12ImGuiBackend final
{
  public:
	explicit D3D12ImGuiBackend(D3D12RenderHardwareInterface& renderHardware) noexcept;

	bool Initialize();
	void BeginFrame() noexcept;
	void Render(NativeGraphicsCommandListHandle commandList, ImDrawData* drawData) noexcept;
	void Shutdown() noexcept;

  private:
	static void AllocateDescriptor(
	    ImGui_ImplDX12_InitInfo* info,
	    D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle);
	static void ReleaseDescriptor(
	    ImGui_ImplDX12_InitInfo* info,
	    D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
	    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);

	static ID3D12Device* ToD3D12Device(NativeGraphicsDeviceHandle handle) noexcept;
	static ID3D12CommandQueue* ToD3D12CommandQueue(NativeGraphicsQueueHandle handle) noexcept;
	static ID3D12DescriptorHeap* ToD3D12DescriptorHeap(NativeDescriptorHeapHandle handle) noexcept;
	static ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept;
	static D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept;
	static D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept;

	D3D12RenderHardwareInterface* m_renderHardware = nullptr;
};