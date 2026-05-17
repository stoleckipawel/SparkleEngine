#pragma once

#include "Device/RenderHardwareInterface.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <memory>
#include <vector>

class VulkanCommandContext;
class VulkanRenderCommandList;
class VulkanRhi;
class VulkanSwapChain;

class VulkanRenderHardwareInterface final : public RenderHardwareInterface
{
  public:
	VulkanRenderHardwareInterface(VulkanRhi& rhi, VulkanSwapChain& swapChain, VulkanCommandContext& commandContext) noexcept;
	~VulkanRenderHardwareInterface() noexcept override;

	VulkanRenderHardwareInterface(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface& operator=(const VulkanRenderHardwareInterface&) = delete;
	VulkanRenderHardwareInterface(VulkanRenderHardwareInterface&&) = delete;
	VulkanRenderHardwareInterface& operator=(VulkanRenderHardwareInterface&&) = delete;

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
	bool InitializeImGuiBackend() override;
	void BeginImGuiFrame() noexcept override;
	void RenderImGuiDrawData(ImDrawData* drawData) noexcept override;
	void ShutdownImGuiBackend() noexcept override;
	std::unique_ptr<RenderBindingLayout> CreateBindingLayout(const RenderBindingLayoutCompileDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateGraphicsPipelineState(const GraphicsPipelineStateDesc& desc) override;
	std::unique_ptr<RenderPipelineState> CreateComputePipelineState(const ComputePipelineStateDesc& desc) override;
	void BindGlobalDescriptorState(RenderCommandList& commandList) const noexcept override;
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
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(std::uint32_t instanceCount) const noexcept override;
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
	void SetCurrentFrameIndex(std::uint32_t frameIndex) noexcept;
	void RebuildSwapChainBackBufferViews() noexcept;

  private:
	struct ResourceViewRecord final
	{
		ERhiResourceViewKind Kind = ERhiResourceViewKind::TextureShaderResource;
		VkImage Image = VK_NULL_HANDLE;
		VkImageView ImageView = VK_NULL_HANDLE;
		bool OwnsImageView = false;
	};

	static void FailRenderingNotImplemented(std::string_view operation) noexcept;
	static RhiResourceViewHandle MakeResourceViewHandle(std::uint32_t index) noexcept;
	static std::uintptr_t EncodeImageViewHandle(VkImageView imageView) noexcept;
	RhiResourceViewHandle AddResourceView(ResourceViewRecord record);
	ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) noexcept;
	const ResourceViewRecord* FindResourceViewRecord(RhiResourceViewHandle view) const noexcept;
	VkImageView CreateImageView(const RhiResourceViewDesc& desc) const;
	VkFormat ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept;
	VkImageAspectFlags ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept;
	RhiResourceViewHandle GetCurrentBackBufferViewHandle() const noexcept;
	void ReleaseAllResourceViews() noexcept;
	void BeginCurrentBackBufferRendering(const float* clearColor, bool clear) noexcept;
	void EndCurrentBackBufferRendering() noexcept;
	void TransitionCurrentBackBuffer(VkCommandBuffer commandBuffer, VkImageLayout newLayout) noexcept;

	VulkanRhi* m_rhi = nullptr;
	VulkanSwapChain* m_swapChain = nullptr;
	VulkanCommandContext* m_commandContext = nullptr;
	std::unique_ptr<RenderDiagnostics> m_diagnostics;
	PerFrameConstantBufferData m_emptyPerFrameConstants = {};
	std::uint32_t m_currentFrameIndex = 0;
	std::vector<ResourceViewRecord> m_resourceViewRecords;
	std::vector<std::uint32_t> m_freeResourceViewIndices;
	std::vector<RhiResourceViewHandle> m_swapChainBackBufferViews;
	std::vector<VkImageLayout> m_swapChainBackBufferLayouts;
	bool m_isPresentRendering = false;
};