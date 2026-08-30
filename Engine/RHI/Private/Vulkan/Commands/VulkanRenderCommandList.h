#pragma once

#include "Commands/RenderCommandList.h"
#include "Commands/RhiCommandRecordingLease.h"
#include "Vulkan/Diagnostics/VulkanDebugEvents.h"
#include "Vulkan/Memory/VulkanRecordingResource.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class VulkanGpuMemoryAllocator;
class VulkanCommandRecordingContext;
class VulkanDescriptorAllocator;
class VulkanDescriptorService;
class VulkanRecordingDescriptorPool;
class VulkanRecordingUploadPage;
class VulkanBindingLayout;
class VulkanRhi;
class VulkanUploadService;
struct VulkanResourceStateMapping;
class VulkanRenderCommandList final : public RenderCommandList
{
public:
	VulkanRenderCommandList();
	~VulkanRenderCommandList() noexcept override;

	RhiGpuVirtualAddress AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) noexcept;

	ERhiBackendApi GetBackendApi() const noexcept override;
	ERhiQueueType GetQueueType() const noexcept override { return m_queueType; }
	VkCommandBuffer GetVulkanCommandBuffer() const noexcept { return m_commandBuffer; }
	bool IsRecording() const noexcept { return m_isRecording; }
	bool IsCoordinatorRecording() const noexcept;
	NativeGraphicsCommandListHandle GetNativeHandle(const RhiNativeInteropRequest& request) const noexcept override;
	bool SupportsDiagnosticScopes() const noexcept override;
	void BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept override;
	void EndDiagnosticScope() noexcept override;
	void InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept override;
	void SetPipeline(const RenderPipeline& pipeline) noexcept override;
	void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override;
	void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override;
	void SetRayTracingBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override;
	void SetRayTracingPipeline(const RayTracingPipeline& pipeline) noexcept override;
	void ResetBoundState() noexcept override;
	void BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindGraphicsAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept override;
	void BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept override;
	void BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override;
	void SetGraphicsPushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override;
	void BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindComputeAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept override;
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept override;
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override;
	void SetComputePushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override;
	void BindRayTracingConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindRayTracingShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindRayTracingUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindRayTracingAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept override;
	void BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept override;
	void BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override;
	void SetRayTracingPushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override;
	void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept override;
	void BindVertexBuffer(const RhiVertexBufferView& view) noexcept override;
	void BindIndexBuffer(const RhiIndexBufferView& view) noexcept override;
	void SetRenderTarget(RhiCpuDescriptorHandle renderTarget, const RhiCpuDescriptorHandle* depthStencil = nullptr) noexcept override;
	void SetRenderTargets(
	    std::uint32_t renderTargetCount,
	    const RhiCpuDescriptorHandle* renderTargets,
	    const RhiCpuDescriptorHandle* depthStencil = nullptr) noexcept override;
	void ClearRenderTarget(RhiCpuDescriptorHandle renderTarget, const float color[4]) noexcept override;
	void ClearDepthStencil(RhiCpuDescriptorHandle depthStencil, float depth, std::uint8_t stencil = 0) noexcept override;
	void EndRasterPass() noexcept override;
	void SetViewport(const RhiViewport& viewport) noexcept override;
	void SetScissorRect(const RhiRect& rect) noexcept override;
	void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override;
	void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept override;
	void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept override;
	void TraceRays(const TraceRaysDesc& desc) noexcept override;
	void BuildBottomLevelAccelerationStructure(
	    const RhiRayTracingGeometryDesc& geometry,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress) noexcept override;
	void BuildTopLevelAccelerationStructure(
	    RhiGpuVirtualAddress instanceDescsGpuAddress,
	    std::uint32_t instanceCount,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress,
	    ERhiClassicTlasBuildMode buildMode = ERhiClassicTlasBuildMode::Build) noexcept override;
	void BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc& desc) noexcept override;
	void CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept override;
	void AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept override;
	void TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept override;
	void UnorderedAccessBarrier(RhiResourceHandle resource) noexcept override;

private:
	friend class VulkanCommandRecordingContext;
	friend class VulkanUploadService;

	struct BufferBinding final
	{
		VkBuffer Buffer = VK_NULL_HANDLE;
		VkDeviceSize Offset = 0;
		VkDeviceSize Range = VK_WHOLE_SIZE;
	};

	struct RecordingResourceUse final
	{
		RhiResourceHandle Resource;
		VulkanRecordingResourceUseToken Token;
	};

	struct ShaderBindingState final
	{
		const VulkanBindingLayout* Layout = nullptr;
		VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> DescriptorSets;
		std::vector<bool> DirtyDescriptorSets;
		std::vector<bool> BoundDescriptorSets;
	};

	void SetRhi(const VulkanRhi* rhi) noexcept { m_rhi = rhi; }
	void SetMemoryAllocator(const VulkanGpuMemoryAllocator* memoryAllocator) noexcept { m_memoryAllocator = memoryAllocator; }
	void SetDescriptorService(const VulkanDescriptorService* descriptorService) noexcept { m_descriptorService = descriptorService; }
	void SetDescriptorAllocator(VulkanDescriptorAllocator* descriptorAllocator) noexcept { m_descriptorAllocator = descriptorAllocator; }
	void SetRecordingDescriptorPool(VulkanRecordingDescriptorPool* descriptorPool) noexcept { m_recordingDescriptorPool = descriptorPool; }
	void SetRecordingUploadPage(VulkanRecordingUploadPage* uploadPage) noexcept { m_recordingUploadPage = uploadPage; }
	void SetRecording(bool recording) noexcept { m_isRecording = recording; }
	void SetRecordingOwner(RhiCommandRecordingOwner owner) noexcept { m_recordingOwner = owner; }
	void SetQueueType(ERhiQueueType queueType) noexcept { m_queueType = queueType; }
	void CloseOpenRendering() noexcept;
	void SetNativeCommandBuffer(
	    VkCommandBuffer commandBuffer,
	    PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
	    PFN_vkCmdEndDebugUtilsLabelEXT endLabel,
	    PFN_vkCmdInsertDebugUtilsLabelEXT insertLabel) noexcept;
	void TrackTransientAllocation(VulkanGpuAllocationRecord& allocation) noexcept;
	void ResolveTransientAllocationUses(RhiSubmissionToken submissionToken) noexcept;
	void AbandonTransientAllocationUses() noexcept;
	void ReleaseTransientAllocationUses(RhiSubmissionToken submissionToken) noexcept;
	void OnResourceTrackingStarted(RhiResourceHandle resource) noexcept override;
	void OnResourceTrackingFinished(RhiResourceHandle resource, RhiSubmissionToken submissionToken) noexcept override;

	static const CompiledBinding* FindBindingByIndex(const VulkanBindingLayout* layout, std::uint32_t bindingIndex) noexcept;
	static VkShaderStageFlags ToVkShaderStages(ShaderStageMask visibilityMask) noexcept;
	VulkanResourceStateMapping ResolveResourceState(ResourceState state) const noexcept;
	static void ConfigurePartitionedTlasInput(
	    const RhiPartitionedTlasDesc& desc,
	    VkPartitionedAccelerationStructureInstancesInputNV& input,
	    VkPartitionedAccelerationStructureFlagsNV& flags) noexcept;
	VkBuffer ResolveBuffer(RhiGpuVirtualAddress gpuAddress) const noexcept;
	BufferBinding ResolveBufferBinding(RhiGpuVirtualAddress gpuAddress) const noexcept;
	bool ResolveResource(RhiResourceHandle resource, VulkanRecordingResource& outResource) const noexcept;
	void RecordBufferTransition(
	    const VulkanRecordingResource& resource,
	    ResourceState before,
	    ResourceState after,
	    const VulkanResourceStateMapping& sourceState,
	    const VulkanResourceStateMapping& destinationState) noexcept;
	void RecordImageTransition(
	    const VulkanRecordingResource& resource,
	    ResourceState before,
	    ResourceState after,
	    const VulkanResourceStateMapping& sourceState,
	    const VulkanResourceStateMapping& destinationState) noexcept;
	VkDeviceAddress ResolveRayTracingBufferAddress(const RhiRayTracingBufferBinding& binding) const noexcept;
	bool ResolveAddress(RhiGpuVirtualAddress address, VulkanRecordingResource& outResource) const noexcept;
	void WriteAccelerationStructureBinding(
	    VkDescriptorSet descriptorSet,
	    const CompiledBinding& binding,
	    RhiResourceHandle resource) noexcept;
	void BeginDynamicRenderingIfNeeded() noexcept;
	void EndDynamicRenderingIfNeeded() noexcept;
	VkDescriptorSet EnsureDescriptorSet(
	    const VulkanBindingLayout* layout,
	    std::uint32_t setIndex,
	    std::vector<VkDescriptorSet>& descriptorSets,
	    std::vector<bool>& boundSets) noexcept;
	void BindDescriptorSet(
	    VkPipelineBindPoint bindPoint,
	    VkPipelineLayout pipelineLayout,
	    std::uint32_t setIndex,
	    VkDescriptorSet descriptorSet) noexcept;
	void CopyDescriptorSet(
	    const VulkanBindingLayout* layout,
	    std::uint32_t setIndex,
	    VkDescriptorSet sourceSet,
	    VkDescriptorSet destinationSet) noexcept;
	void MarkDescriptorSetDirty(std::uint32_t setIndex, std::vector<bool>& dirtySets) noexcept;
	static void InitializeShaderBindingState(const VulkanBindingLayout& layout, ShaderBindingState& state);
	static void ReserveShaderBindingState(ShaderBindingState& state, std::size_t descriptorSetCount);
	static void ClearShaderBindingDescriptors(ShaderBindingState& state) noexcept;
	static void ResetShaderBindingState(ShaderBindingState& state) noexcept;
	void BindShaderBuffer(ShaderBindingState& state, std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;
	void BindShaderAccelerationStructure(ShaderBindingState& state, std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept;
	void BindShaderDescriptorTable(ShaderBindingState& state, std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept;
	void BindShaderDescriptorTable(ShaderBindingState& state, std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept;
	void SetShaderPushConstants(
	    const ShaderBindingState& state,
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept;
	void FlushShaderDescriptorSets(VkPipelineBindPoint bindPoint, ShaderBindingState& state) noexcept;
	void FlushGraphicsDescriptorSets() noexcept;
	void FlushComputeDescriptorSets() noexcept;
	void FlushRayTracingDescriptorSets() noexcept;
	VkImageAspectFlags ResolveDepthStencilAspectMask(VkImageView imageView) const noexcept;

	static constexpr std::uint32_t MaxRenderTargets = 8;

	const VulkanRhi* m_rhi = nullptr;
	const VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	const VulkanDescriptorService* m_descriptorService = nullptr;
	VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
	VulkanRecordingDescriptorPool* m_recordingDescriptorPool = nullptr;
	VulkanRecordingUploadPage* m_recordingUploadPage = nullptr;
	ERhiQueueType m_queueType = ERhiQueueType::Graphics;
	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	ShaderBindingState m_graphicsBindings;
	ShaderBindingState m_computeBindings;
	ShaderBindingState m_rayTracingBindings;
	const RayTracingPipeline* m_boundRayTracingPipeline = nullptr;
	std::vector<RhiDescriptorTableBinding> m_retainedDescriptorTables;
	std::vector<RhiGpuDescriptorHandle> m_retainedDescriptorHandles;
	std::vector<VkBuffer> m_retainedDescriptorBuffers;
	std::vector<RecordingResourceUse> m_recordingResourceUses;
	std::vector<VulkanGpuAllocationRecord*> m_transientAllocationUses;
	std::size_t m_recordingResourceReleaseIndex = 0;
	VulkanDebugEventFunctions m_debugEvents = {};
	std::array<VkImageView, MaxRenderTargets> m_renderTargets = {};
	std::uint32_t m_renderTargetCount = 0;
	VkImageView m_depthStencil = VK_NULL_HANDLE;
	VkImageAspectFlags m_depthStencilAspectMask = 0;
	VkRect2D m_scissorRect = {};
	bool m_hasScissorRect = false;
	bool m_dynamicRenderingActive = false;
	bool m_isRecording = false;
	RhiCommandRecordingOwner m_recordingOwner = {};
};
