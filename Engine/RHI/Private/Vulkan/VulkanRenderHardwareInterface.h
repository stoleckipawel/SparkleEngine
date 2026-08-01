#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <vector>

class VulkanCommandRecordingContext;
class VulkanUploadService;
class VulkanCaptureService;
class VulkanRenderCommandList;
class VulkanDescriptorService;
class VulkanGpuMemoryAllocator;
class VulkanImGuiBackend;
class VulkanInteropService;
class VulkanRayTracingServices;
class VulkanRenderDeviceServices;
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
	    VulkanGpuMemoryAllocator& memoryAllocator) noexcept;
	~VulkanRenderHardwareInterface() noexcept override;

	VulkanRenderHardwareInterface(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface& operator=(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface(VulkanRenderHardwareInterface&&) = delete;
	VulkanRenderHardwareInterface& operator=(VulkanRenderHardwareInterface&&) = delete;

	const RhiCapabilities& GetCapabilities() const noexcept override { return m_capabilities; }
	std::uint32_t GetCurrentFrameIndex() const noexcept override;
	RhiResourceService& GetResourceService() noexcept override;
	const RhiResourceService& GetResourceService() const noexcept override;
	RhiDescriptorService& GetDescriptorService() noexcept override;
	const RhiDescriptorService& GetDescriptorService() const noexcept override;
	RhiPipelineService& GetPipelineService() noexcept override;
	RhiUploadService& GetUploadService() noexcept override;
	const RhiUploadService& GetUploadService() const noexcept override;
	RhiRayTracingService& GetRayTracingService() noexcept override;
	const RhiRayTracingService& GetRayTracingService() const noexcept override;
	void WaitForIdle() noexcept;
	RhiInteropService& GetInteropService() noexcept override;
	const RhiInteropService& GetInteropService() const noexcept override;
	RhiCaptureService& GetCaptureService() noexcept override;
	RenderDiagnostics& GetDiagnostics() noexcept override;
	const RenderDiagnostics& GetDiagnostics() const noexcept override;
	RhiPresentationService& GetPresentationService() noexcept override;
	const RhiPresentationService& GetPresentationService() const noexcept override;
	NativeGraphicsDeviceHandle GetDeviceHandle() const noexcept;
	NativeGraphicsQueueHandle GetGraphicsQueueHandle() const noexcept;
	RenderCommandList& GetGraphicsCommandList(std::uint32_t frameIndex) noexcept;
	RenderCommandList& GetCommandList(ERhiQueueType queueType, std::uint32_t frameIndex) noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;
	RhiImGuiRenderer& GetImGuiRenderer() noexcept;
	RhiViewport GetBackBufferViewport() const noexcept;
	RhiRect GetBackBufferScissorRect() const noexcept;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept;
	RhiResourceHandle GetBackBufferResource() const noexcept;
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
	friend class VulkanInteropService;
	friend class VulkanRenderDeviceServices;

	RhiCapabilities BuildCapabilities() const noexcept;
	RhiBackendDiagnosticsSupport BuildBackendDiagnosticsSupport() const noexcept;
	RhiBackendMemorySupport BuildBackendMemorySupport() const noexcept;
	RhiFormatSupport QueryFormatSupport(PixelFormat format) const noexcept;
	RhiResourceViewHandle GetCurrentBackBufferViewHandle() const noexcept;
	void BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept;
	void EndCurrentBackBufferRendering() noexcept;
	void PrepareCurrentBackBufferForPresentation(VulkanRenderCommandList& commandList) noexcept;
	void TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, ResourceState newState) noexcept;
	void SetCommandRecordingContext(
	    VulkanCommandRecordingContext& commandRecordingContext) noexcept;

	std::unique_ptr<VulkanInteropService> m_interopService;
	std::unique_ptr<VulkanCaptureService> m_captureService;
	std::unique_ptr<RhiPresentationService> m_presentationService;
	std::unique_ptr<RhiPipelineService> m_pipelineService;
	std::unique_ptr<VulkanResourceService> m_resourceService;
	std::unique_ptr<VulkanRayTracingServices> m_rayTracingServices;
	VulkanRhi* m_rhi = nullptr;
	VulkanSwapChain* m_swapChain = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanCommandRecordingContext* m_commandRecordingContext = nullptr;
	std::unique_ptr<VulkanDescriptorService> m_descriptorService;
	std::unique_ptr<VulkanUploadService> m_uploadService;
	std::unique_ptr<VulkanSamplerLibrary> m_samplerLibrary;
	std::unique_ptr<VulkanImGuiBackend> m_imguiBackend;
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	RhiCapabilities m_capabilities;
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<VkImageLayout> m_swapChainBackBufferLayouts;
};
