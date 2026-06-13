#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include <memory>

#include "Config/RenderConfig.h"
#include "Resources/RenderConstantBufferData.h"
#include "Resources/RhiUploadService.h"
#include "D3D12ConstantBuffer.h"

class Timer;
class Window;
class D3D12Rhi;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;

class D3D12ConstantBufferManager final : public RhiUploadService
{
  public:
	D3D12ConstantBufferManager(
	    Timer& timer,
	    D3D12Rhi& rhi,
	    Window& window,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12FrameResourceManager& frameResourceManager,
	    D3D12SwapChain& swapChain);
	~D3D12ConstantBufferManager() noexcept;

	D3D12ConstantBufferManager(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager& operator=(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager(D3D12ConstantBufferManager&&) = delete;
	D3D12ConstantBufferManager& operator=(D3D12ConstantBufferManager&&) = delete;

	D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameGpuAddress() const;
	RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept override { return GetPerFrameGpuAddress(); }
	const PerFrameConstantBufferData& GetPerFrameData() const noexcept;
	const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept override { return GetPerFrameData(); }

	void UpdatePerFrame(std::uint32_t viewModeIndex, std::uint32_t viewportWidth, std::uint32_t viewportHeight);
	D3D12_GPU_VIRTUAL_ADDRESS AllocateUniform(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override
	{
		return AllocateUniform(data, sizeInBytes);
	}
	D3D12_GPU_VIRTUAL_ADDRESS AllocatePerView(const PerViewConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) override { return AllocatePerView(data); }

	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectVS(const PerObjectVSConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) override
	{
		return UpdatePerObjectVS(data);
	}

	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectPS(const PerObjectPSConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) override
	{
		return UpdatePerObjectPS(data);
	}

  private:
	std::unique_ptr<D3D12ConstantBuffer<PerFrameConstantBufferData>> m_perFrameCB[RenderConfig::FramesInFlight];
	PerFrameConstantBufferData m_perFrameData[RenderConfig::FramesInFlight] = {};

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
};
