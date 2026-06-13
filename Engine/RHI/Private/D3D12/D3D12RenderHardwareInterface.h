#pragma once

#include "Config/RenderConfig.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Device/RenderHardwareInterface.h"

#include <array>
#include <memory>
#include <string>

class D3D12DescriptorHeapManager;
class D3D12DescriptorService;
class D3D12ConstantBufferManager;
class D3D12CaptureService;
class D3D12DiagnosticsService;
class D3D12ImGuiBackend;
class D3D12InteropService;
class D3D12GpuMemoryAllocator;
class D3D12PipelineService;
class D3D12PresentationService;
class D3D12RenderCommandList;
class D3D12ResourceService;
class D3D12Rhi;
class D3D12RayTracingServices;
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
	RhiInteropService& GetInteropService() noexcept override;
	const RhiInteropService& GetInteropService() const noexcept override;
	RhiCaptureService& GetCaptureService() noexcept override;
	RhiDiagnosticsService& GetDiagnosticsService() noexcept override;
	const RhiDiagnosticsService& GetDiagnosticsService() const noexcept override;
	RhiPresentationService& GetPresentationService() noexcept override;
	const RhiPresentationService& GetPresentationService() const noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept override;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept override;
	bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept override;
	bool CaptureTextureToBmp(
	    NativeResourceHandle resource,
	    std::uint32_t width,
	    std::uint32_t height,
	    const std::filesystem::path& outputPath) noexcept override;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept override;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept override;
	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc) override;
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
	std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName) override;
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
	bool CreateStructuredBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiResourceViewHandle& outView) override;
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
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) override;
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
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept override;
	std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept override;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept override;
	void BeginPresentOverlayPass() noexcept override;
	void EndPresentRenderPass() noexcept override;
	PixelFormat GetPresentColorFormat() const noexcept override;
	void SetSamplerTableHandle(RhiDescriptorTableHandle samplerTableHandle) noexcept;

  private:
	friend class D3D12RenderCommandList;

	D3D12_CPU_DESCRIPTOR_HANDLE ResolveDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	D3D12_GPU_DESCRIPTOR_HANDLE ResolveDescriptorTableGpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	RhiCapabilities BuildCapabilities() const noexcept;
	RhiFormatSupport QueryFormatSupport(PixelFormat format) const noexcept;

	std::unique_ptr<D3D12InteropService> m_interopService;
	std::unique_ptr<D3D12CaptureService> m_captureService;
	std::unique_ptr<D3D12DiagnosticsService> m_diagnosticsService;
	std::unique_ptr<D3D12PresentationService> m_presentationService;
	std::unique_ptr<D3D12PipelineService> m_pipelineService;
	std::unique_ptr<D3D12ResourceService> m_resourceService;
	std::unique_ptr<D3D12RayTracingServices> m_rayTracingServices;
	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	D3D12ConstantBufferManager* m_constantBufferManager = nullptr;
	std::unique_ptr<D3D12DescriptorService> m_descriptorService;
	std::unique_ptr<D3D12ImGuiBackend> m_imguiBackend;
	RhiCapabilities m_capabilities;
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	std::array<std::unique_ptr<RenderCommandList>, RenderConfig::FramesInFlight> m_commandLists;
};
