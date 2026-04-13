#pragma once

#include "Interop/RenderHardwareInterface.h"

class D3D12DescriptorHeapManager;
class D3D12Rhi;
class D3D12SwapChain;

class SPARKLE_RHI_API D3D12RenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	D3D12RenderHardwareInterface(
	    D3D12Rhi& rhi,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12SwapChain& swapChain) noexcept;
	~D3D12RenderHardwareInterface() noexcept override = default;

	D3D12RenderHardwareInterface(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface& operator=(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface(D3D12RenderHardwareInterface&&) = delete;
	D3D12RenderHardwareInterface& operator=(D3D12RenderHardwareInterface&&) = delete;

	RhiBackendApi GetBackendApi() const noexcept override;
	std::uint32_t GetCurrentFrameIndex() const noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept override;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept override;
	NativeGraphicsCommandListHandle GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept override;
	NativeDescriptorHeapHandle GetShaderResourceHeapHandle() const noexcept override;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) override;
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept override;
	void BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4]) const noexcept override;
	void EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept override;
	std::uint32_t GetPresentColorFormat() const noexcept override;

  private:
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
};