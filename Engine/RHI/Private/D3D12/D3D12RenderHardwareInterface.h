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
	~D3D12RenderHardwareInterface() noexcept;

	D3D12RenderHardwareInterface(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface& operator=(const D3D12RenderHardwareInterface&) = delete;
	D3D12RenderHardwareInterface(D3D12RenderHardwareInterface&&) = delete;
	D3D12RenderHardwareInterface& operator=(D3D12RenderHardwareInterface&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept { return m_capabilities; }
	ERhiBackendApi GetBackendApi() const noexcept;
	CookedShaderBinaryFormat GetRequiredShaderBinaryFormat() const noexcept;
	std::uint32_t GetCurrentFrameIndex() const noexcept;
	RhiResourceService& GetResourceService() noexcept;
	const RhiResourceService& GetResourceService() const noexcept;
	RhiDescriptorService& GetDescriptorService() noexcept;
	const RhiDescriptorService& GetDescriptorService() const noexcept;
	RhiPipelineService& GetPipelineService() noexcept;
	RhiUploadService& GetUploadService() noexcept;
	const RhiUploadService& GetUploadService() const noexcept;
	RhiRayTracingService& GetRayTracingService() noexcept;
	const RhiRayTracingService& GetRayTracingService() const noexcept;
	void WaitForIdle() noexcept;
	RhiInteropService& GetInteropService() noexcept;
	const RhiInteropService& GetInteropService() const noexcept;
	RhiCaptureService& GetCaptureService() noexcept;
	RhiDiagnosticsService& GetDiagnosticsService() noexcept;
	const RhiDiagnosticsService& GetDiagnosticsService() const noexcept;
	RhiPresentationService& GetPresentationService() noexcept;
	const RhiPresentationService& GetPresentationService() const noexcept;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept;
	bool UpgradePresentationInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept;
	bool CaptureTextureToBmp(
	    NativeResourceHandle resource,
	    std::uint32_t width,
	    std::uint32_t height,
	    const std::filesystem::path& outputPath) noexcept;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;
	RenderDiagnostics& GetDiagnostics() noexcept;
	const RenderDiagnostics& GetDiagnostics() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc);
	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept;
	ID3D12DescriptorHeap* GetD3D12ShaderResourceDescriptorHeap() const noexcept;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType);
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount);
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle);
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept;
	const PerFrameConstantBufferData& GetPerFrameConstantData() const noexcept;
	RhiGpuVirtualAddress GetPerFrameConstantGpuAddress() const noexcept;
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes);
	RhiGpuVirtualAddress AllocatePerViewConstantBuffer(const PerViewConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectVertexConstants(const PerObjectVSConstantBufferData& data);
	RhiGpuVirtualAddress AllocatePerObjectPixelConstants(const PerObjectPSConstantBufferData& data);
	RhiDescriptorTableBinding GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept;
	RhiViewport GetBackBufferViewport() const noexcept;
	RhiRect GetBackBufferScissorRect() const noexcept;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept;
	NativeResourceHandle GetBackBufferResource() const noexcept;
	std::unique_ptr<Texture> CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateTextureResource(
	    const RhiTextureResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateBufferResource(
	    const RhiBufferResourceDesc& desc,
	    ResourceState initialState,
	    RhiMemoryCategory category,
	    RhiMemoryResidencyClass residencyClass,
	    std::wstring_view debugName);
	bool CreateVertexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiVertexBufferView& outView);
	bool CreateStructuredBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    std::uint32_t strideInBytes,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiResourceViewHandle& outView);
	bool CreateIndexBuffer(
	    const void* data,
	    std::size_t sizeInBytes,
	    RhiIndexFormat format,
	    std::wstring_view debugName,
	    RhiOwnedResourceHandle& outResource,
	    RhiIndexBufferView& outView);
	void ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept;
	NativeResourceHandle GetNativeResource(RhiOwnedResourceHandle resource) const noexcept;
	RhiGpuVirtualAddress GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount) const noexcept;
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	RhiResourceAllocationInfo GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept;
	RhiResourceAllocationInfo GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept;
	RhiOwnedMemoryBlockHandle CreateTransientMemoryBlock(
	    RhiTransientAllocationPool pool,
	    std::uint64_t sizeInBytes,
	    std::uint64_t alignment,
	    std::wstring_view debugName);
	void ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept;
	RhiOwnedResourceHandle CreateAliasingTextureResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientTextureAllocationDesc& desc,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateAliasingBufferResource(
	    RhiOwnedMemoryBlockHandle memoryBlock,
	    std::uint64_t memoryBlockOffset,
	    const RhiTransientBufferAllocationDesc& desc,
	    std::wstring_view debugName);
	RhiResourceViewHandle CreateResourceView(const RhiResourceViewDesc& desc);
	void ReleaseResourceView(RhiResourceViewHandle view) noexcept;
	RhiCpuDescriptorHandle GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept;
	RhiGpuDescriptorHandle GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept;
	NativeTextureViewInfo GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept;
	std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept;
	bool SupportsUnorderedAccess(NativeResourceHandle resource) const noexcept;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept;
	void BeginPresentOverlayPass() noexcept;
	void EndPresentRenderPass() noexcept;
	PixelFormat GetPresentColorFormat() const noexcept;
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
