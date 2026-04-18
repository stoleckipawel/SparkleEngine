#pragma once

#include "../Config/RenderConfig.h"
#include "Descriptors/D3D12DescriptorHandle.h"
#include "../Interop/RenderHardwareInterface.h"

#include <array>
#include <memory>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12ConstantBufferManager;
class D3D12Rhi;
class D3D12SamplerLibrary;
class D3D12SwapChain;

class SPARKLE_RHI_API D3D12RenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	D3D12RenderHardwareInterface(
	    D3D12Rhi& rhi,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12SwapChain& swapChain,
	    D3D12ConstantBufferManager& constantBufferManager) noexcept;
	~D3D12RenderHardwareInterface() noexcept override = default;

	D3D12RenderHardwareInterface(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface& operator=(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface(D3D12RenderHardwareInterface&&) = delete;
	D3D12RenderHardwareInterface& operator=(D3D12RenderHardwareInterface&&) = delete;

	RhiBackendApi GetBackendApi() const noexcept override;
	std::uint32_t GetCurrentFrameIndex() const noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept override;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	NativeGraphicsCommandListHandle GetGraphicsCommandListHandle(std::uint32_t frameIndex) const noexcept override;
	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) override;
	void SetShaderVisibleDescriptorHeaps(RenderCommandList& commandList) const noexcept override;
	NativeDescriptorHeapHandle GetShaderResourceHeapHandle() const noexcept override;
	RhiDescriptorAllocation AllocateDescriptor(RhiDescriptorHeapType heapType) override;
	void ReleaseDescriptor(RhiDescriptorHeapType heapType, const RhiDescriptorAllocation& allocation) noexcept override;
	RhiDescriptorTableHandle AllocateDescriptorTable(RhiDescriptorHeapType heapType, std::uint32_t descriptorCount) override;
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept override;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept override;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) override;
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept override;
	const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept override;
	RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept override;
	RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) override;
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) override;
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) override;
	RhiDescriptorTableHandle GetSamplerTableHandle() const noexcept override;
	RhiViewport GetBackBufferViewport() const noexcept override;
	RhiRect GetBackBufferScissorRect() const noexcept override;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept override;
	NativeResourceHandle GetBackBufferResource() const noexcept override;
	std::unique_ptr<Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const override;
	bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView) override;
	bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView) override;
	void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept override;
	NativeResourceHandle GetNativeResource(RhiOwnedResourceHandle resource) const noexcept override;
	RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept override;
	RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept override;
	RhiOwnedHeapHandle CreateOwnedHeap(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) override;
	void ReleaseOwnedHeap(RhiOwnedHeapHandle heap) noexcept override;
	RhiOwnedResourceHandle CreatePlacedTextureResource(
	    RhiOwnedHeapHandle heap,
	    std::uint64_t heapOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePlacedBufferResource(
	    RhiOwnedHeapHandle heap,
	    std::uint64_t heapOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) override;
	void CreateRenderTargetView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) override;
	void CreateDepthStencilView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) override;
	void CreateTextureShaderResourceView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) override;
	void CreateTextureUnorderedAccessView(NativeResourceHandle resource, PixelFormat format, RhiCpuDescriptorHandle destination) override;
	void CreateBufferShaderResourceView(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    RhiCpuDescriptorHandle destination) override;
	void CreateBufferUnorderedAccessView(
	    NativeResourceHandle resource,
	    std::uint64_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    RhiCpuDescriptorHandle destination) override;
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept override;
	void TransitionResource(
	    NativeGraphicsCommandListHandle commandList,
	    NativeResourceHandle resource,
	    ResourceState before,
	    ResourceState after) const noexcept override;
	void BeginPresentRenderPass(NativeGraphicsCommandListHandle commandList, const float clearColor[4]) const noexcept override;
	void EndPresentRenderPass(NativeGraphicsCommandListHandle commandList) const noexcept override;
	PixelFormat GetPresentColorFormat() const noexcept override;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;

  private:
	class D3D12RenderCommandList;

	struct DescriptorTableRecord
	{
		RhiDescriptorHeapType heapType = RhiDescriptorHeapType::ShaderResource;
		std::uint32_t descriptorCount = 0;
		D3D12DescriptorHandle nativeHandle;

		bool IsAllocated() const noexcept { return nativeHandle.IsValid(); }
	};

	D3D12_CPU_DESCRIPTOR_HANDLE ResolveDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveDescriptorTableGpuHandle(RhiDescriptorTableHandle tableHandle) const noexcept;
	DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	static D3D12_DESCRIPTOR_HEAP_TYPE ToNativeDescriptorHeapType(RhiDescriptorHeapType heapType) noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	RhiDescriptorTableHandle m_samplerTableHandle = {};
	std::array<std::unique_ptr<RenderCommandList>, RenderConfig::FramesInFlight> m_commandLists;
	std::vector<DescriptorTableRecord> m_descriptorTableRecords;
	std::vector<std::uint32_t> m_freeDescriptorTableIndices;
};