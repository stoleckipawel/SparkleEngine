#pragma once

#include "Interop/RhiNativeHandles.h"
#include "UI/RhiImGuiRenderer.h"

#include <d3d12.h>

struct ImDrawData;
struct ImGuiContext;
struct ImGui_ImplDX12_InitInfo;

class D3D12RenderHardwareInterface;

class D3D12ImGuiBackend final : public RhiImGuiRenderer
{
  public:
	explicit D3D12ImGuiBackend(D3D12RenderHardwareInterface& renderHardwareInterface) noexcept;

	void Initialize() override;
	void BeginFrame() noexcept override;
	std::uint64_t ResolveTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	void RenderDrawData(ImDrawData* drawData) noexcept override;
	void ReleaseTexture(ImTextureData& texture) noexcept override;
	void Render(NativeGraphicsCommandListHandle commandList, ImDrawData* drawData) noexcept;
	void Shutdown() noexcept override;

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
	static ID3D12GraphicsCommandList* ToD3D12GraphicsCommandList(NativeGraphicsCommandListHandle handle) noexcept;
	static D3D12_CPU_DESCRIPTOR_HANDLE ToD3D12CpuDescriptor(RhiCpuDescriptorHandle handle) noexcept;
	static D3D12_GPU_DESCRIPTOR_HANDLE ToD3D12GpuDescriptor(RhiGpuDescriptorHandle handle) noexcept;
	ImGuiContext* ActivateContext() const noexcept;
	static void RestoreContext(ImGuiContext* context) noexcept;

	D3D12RenderHardwareInterface& m_renderHardwareInterface;
	ImGuiContext* m_imguiContext = nullptr;
	bool m_ownsContext = false;
};
