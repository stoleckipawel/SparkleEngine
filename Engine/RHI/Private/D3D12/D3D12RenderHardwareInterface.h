#pragma once

#include "Config/RenderConfig.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Device/RenderHardwareInterface.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"

#include <array>
#include <memory>
#include <string>
#include <vector>

class D3D12DescriptorHeapManager;
class D3D12ConstantBufferManager;
class D3D12ImGuiBackend;
class D3D12GpuMemoryAllocator;
class D3D12RenderCommandList;
class D3D12Rhi;
class D3D12SamplerLibrary;
class D3D12SwapChain;
class RhiImGuiRenderer;

class D3D12RenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	D3D12RenderHardwareInterface(
	    D3D12Rhi& rhi,
	    D3D12GpuMemoryAllocator& memoryAllocator,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12SwapChain& swapChain,
	    D3D12ConstantBufferManager& constantBufferManager) noexcept;
	~D3D12RenderHardwareInterface() noexcept override;

	D3D12RenderHardwareInterface(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface& operator=(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface(D3D12RenderHardwareInterface&&) = delete;
	D3D12RenderHardwareInterface& operator=(D3D12RenderHardwareInterface&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept override { return m_capabilities; }
	ERhiBackendApi GetBackendApi() const noexcept override;
	CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept override;
	std::uint32_t GetCurrentFrameIndex() const noexcept override;
	void WaitForIdle() noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept override;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept override;
	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) override;
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept override;
	ID3D12DescriptorHeap* GetD3D12ShaderResourceDescriptorHeap() const noexcept;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType) override;
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept override;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount) override;
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept override;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept override;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle) override;
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept override;
	const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept override;
	RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept override;
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) override;
	RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data) override;
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data) override;
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data) override;
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept override;
	RhiViewport GetBackBufferViewport() const noexcept override;
	RhiRect GetBackBufferScissorRect() const noexcept override;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept override;
	NativeResourceHandle GetBackBufferResource() const noexcept override;
	std::unique_ptr<Texture> CreateTextureFromPath(const std::filesystem::path& texturePath) const override;
	RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName) override;
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
	RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount) const noexcept override;
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override;
	RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept override;
	RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept override;
	RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName) override;
	void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept override;
	RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName) override;
	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc) override;
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept override;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept override;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept override;
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept override;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept override;
	void BeginPresentOverlayPass() noexcept override;
	void EndPresentRenderPass() noexcept override;
	PixelFormat GetPresentColorFormat() const noexcept override;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;

  private:
	friend class D3D12RenderCommandList;

	struct DescriptorTableRecord
	{
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		std::uint32_t descriptorCount = 0;
		D3D12DescriptorHandle nativeHandle;

		bool IsAllocated() const noexcept { return nativeHandle.IsValid(); }
	};

	struct ResourceViewRecord
	{
		ERhiResourceViewKind kind = ERhiResourceViewKind::TextureShaderResource;
		ERhiDescriptorAllocatorType descriptorType = ERhiDescriptorAllocatorType::ShaderResource;
		RhiDescriptorAllocation descriptorAllocation = {};

		bool IsAllocated() const noexcept { return descriptorAllocation.IsValid(); }
	};

	struct PendingOwnedResourceRelease
	{
		std::unique_ptr<D3D12GpuAllocationRecord> Record;
		std::uint64_t RetireFenceValue = 0;
	};

	struct PendingOwnedMemoryBlockRelease
	{
		std::unique_ptr<D3D12GpuHeapRecord> Record;
		std::uint64_t RetireFenceValue = 0;
	};

	D3D12_CPU_DESCRIPTOR_HANDLE ResolveDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveDescriptorTableGpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	static ERhiDescriptorAllocatorType ResolveResourceViewDescriptorAllocatorType(ERhiResourceViewKind kind) noexcept;
	static std::wstring CopyDebugName(std::wstring_view debugName, std::wstring_view fallbackName);
	static RhiOwnedResourceHandle WrapOwnedResource(std::unique_ptr<D3D12GpuAllocationRecord> record) noexcept;
	static RhiOwnedResourceHandle WrapOwnedResource(
	    Microsoft::WRL::ComPtr<ID3D12Resource>&& resource,
	    std::wstring debugName) noexcept;
	static RhiOwnedMemoryBlockHandle WrapOwnedMemoryBlock(std::unique_ptr<D3D12GpuHeapRecord> record) noexcept;
	static bool ResourceSupportsUnorderedAccess(ID3D12Resource* resource) noexcept;
	RhiCapabilities BuildCapabilities() const noexcept;
	RhiFormatSupport QueryFormatSupport(PixelFormat format) const noexcept;
	bool WriteD3D12ResourceViewDescriptor(const RhiResourceViewDesc& desc, RhiCpuDescriptorHandle destination) noexcept;
	void DrainCompletedOwnedResourceReleases() noexcept;
	DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) noexcept;
	const DescriptorTableRecord* FindDescriptorTableRecord(RhiDescriptorTableHandle tableHandle) const noexcept;
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12GpuMemoryAllocator* m_memoryAllocator = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	std::unique_ptr<D3D12ImGuiBackend> m_imguiBackend;
	RhiCapabilities m_capabilities;
	RhiDescriptorTableHandle m_samplerTableHandle = {};
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	std::array<std::unique_ptr<RenderCommandList>, RenderConfig::FramesInFlight> m_commandLists;
	std::vector<DescriptorTableRecord> m_descriptorTableRecords;
	std::vector<std::uint32_t> m_freeDescriptorTableIndices;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::vector<PendingOwnedResourceRelease> m_pendingOwnedResourceReleases;
	std::vector<PendingOwnedMemoryBlockRelease> m_pendingOwnedMemoryBlockReleases;
};
