#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <vector>

class VulkanCommandContext;
class VulkanConstantBufferManager;
class VulkanCaptureService;
class VulkanDescriptorManager;
class VulkanGpuMemoryAllocator;
class VulkanImGuiBackend;
class VulkanInteropService;
class VulkanPipelineService;
class VulkanPresentationService;
class VulkanRayTracingServices;
class VulkanRenderCommandList;
class VulkanResourceService;
class VulkanRhi;
class VulkanSamplerLibrary;
class VulkanSwapChain;
class RhiImGuiRenderer;

class VulkanRenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	VulkanRenderHardwareInterface(
	    VulkanRhi& rhi,
	    VulkanSwapChain& swapChain,
	    VulkanCommandContext& commandContext,
	    VulkanGpuMemoryAllocator& memoryAllocator) noexcept;
	~VulkanRenderHardwareInterface() noexcept;

	VulkanRenderHardwareInterface(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface& operator=(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface(VulkanRenderHardwareInterface&&) = delete;
	VulkanRenderHardwareInterface& operator=(VulkanRenderHardwareInterface&&) = delete;

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
	RenderDiagnostics& GetDiagnostics() noexcept;
	const RenderDiagnostics& GetDiagnostics() const noexcept;
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
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	std::unique_ptr<RenderBindingSet> CreateBindingSet(const RenderBindingSetDesc& desc);
	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc);
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc);
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept;
	RhiDescriptorAllocation AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType);
	void ReleaseDescriptor(ERhiDescriptorAllocatorType descriptorType, const RhiDescriptorAllocation& allocation) noexcept;
	RhiDescriptorTableHandle AllocateDescriptorTable(ERhiDescriptorAllocatorType descriptorType, std::uint32_t descriptorCount);
	RhiCpuDescriptorHandle GetDescriptorTableCpuHandle(RhiDescriptorTableHandle tableHandle, std::uint32_t descriptorIndex = 0)
	    const noexcept;
	void ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept;
	void AllocateShaderResourceDescriptor(RhiCpuDescriptorHandle& outCpuHandle, RhiGpuDescriptorHandle& outGpuHandle);
	void ReleaseShaderResourceDescriptor(RhiCpuDescriptorHandle cpuHandle, RhiGpuDescriptorHandle gpuHandle) noexcept;
	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes);
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
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept;
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
	VkInstance GetVulkanInstance() const noexcept;
	VkPhysicalDevice GetVulkanPhysicalDevice() const noexcept;
	VkDevice GetVulkanDevice() const noexcept;
	VkQueue GetVulkanGraphicsQueue() const noexcept;
	std::uint32_t GetVulkanGraphicsQueueFamilyIndex() const noexcept;
	std::uint32_t GetVulkanApiVersion() const noexcept;
	std::uint32_t GetSwapChainBackBufferCount() const noexcept;
	VkFormat GetNativeBackBufferFormat() const noexcept;
	void SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept;
	void ResetTransientFrameResources() noexcept;
	void RebuildSwapChainBackBufferViews() noexcept;

  private:
	RhiCapabilities BuildCapabilities() const noexcept;
	RhiFormatSupport QueryFormatSupport(PixelFormat format) const noexcept;
	RhiResourceViewHandle GetCurrentBackBufferViewHandle() const noexcept;
	void BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept;
	void EndCurrentBackBufferRendering() noexcept;
	void TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, ResourceState newState) noexcept;

	std::unique_ptr<VulkanInteropService> m_interopService;
	std::unique_ptr<VulkanCaptureService> m_captureService;
	std::unique_ptr<VulkanPresentationService> m_presentationService;
	std::unique_ptr<VulkanPipelineService> m_pipelineService;
	std::unique_ptr<VulkanResourceService> m_resourceService;
	std::unique_ptr<VulkanRayTracingServices> m_rayTracingServices;
	VulkanRhi* m_rhi = nullptr;
	VulkanSwapChain* m_swapChain = nullptr;
	VulkanCommandContext* m_commandContext = nullptr;
	std::unique_ptr<VulkanDescriptorManager> m_descriptorManager;
	std::unique_ptr<VulkanConstantBufferManager> m_constantBufferManager;
	std::unique_ptr<VulkanSamplerLibrary> m_samplerLibrary;
	std::unique_ptr<VulkanImGuiBackend> m_imguiBackend;
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	RhiCapabilities m_capabilities;
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<VkImageLayout> m_swapChainBackBufferLayouts;
	bool m_isPresentRendering = false;
};
