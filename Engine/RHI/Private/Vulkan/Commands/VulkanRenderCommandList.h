#pragma once

#include "Commands/RenderCommandList.h"
#include "Vulkan/Diagnostics/VulkanDebugEvents.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

class VulkanGpuMemoryAllocator;
class VulkanDescriptorAllocator;
class VulkanBindingLayout;
class VulkanRhi;
class VulkanRenderCommandList final : public RenderCommandList
{
  public:
	void SetRhi(const VulkanRhi* rhi) noexcept { m_rhi = rhi; }
	void SetMemoryAllocator(const VulkanGpuMemoryAllocator* memoryAllocator) noexcept { m_memoryAllocator = memoryAllocator; }
	void SetDescriptorAllocator(VulkanDescriptorAllocator* descriptorAllocator) noexcept { m_descriptorAllocator = descriptorAllocator; }
	void CloseOpenRendering() noexcept;
	void SetNativeCommandBuffer(
	    VkCommandBuffer commandBuffer,
	    PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
	    PFN_vkCmdEndDebugUtilsLabelEXT endLabel,
	    PFN_vkCmdInsertDebugUtilsLabelEXT insertLabel) noexcept;

	ERhiBackendApi GetBackendApi() const noexcept override;
	NativeGraphicsCommandListHandle GetNativeHandle() const noexcept override;
	bool SupportsDiagnosticScopes() const noexcept override;
	void BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept override;
	void EndDiagnosticScope() noexcept override;
	void InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept override;
	void SetPipelineState(const RenderPipelineState& pipelineState) noexcept override;
	void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override;
	void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept override;
	void BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
	void BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept override;
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
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept override;
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept override;
	void SetComputePushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept override;
	void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept override;
	void BindVertexBuffer(const RhiVertexBufferView& view) noexcept override;
	void BindIndexBuffer(const RhiIndexBufferView& view) noexcept override;
	void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept override;
	void SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept
	    override;
	void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept override;
	void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil = 0) noexcept override;
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
	void BuildBottomLevelAccelerationStructure(
	    const RhiRayTracingGeometryDesc& geometry,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress) noexcept override;
	void BuildTopLevelAccelerationStructure(
	    RhiGpuVirtualAddress instanceDescsGpuAddress,
	    std::uint32_t instanceCount,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress) noexcept override;
	void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept override;
	void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept override;
	void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept override;
	void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept override;

  private:
	static const CompiledBinding* FindBindingByIndex(const VulkanBindingLayout* layout, std::uint32_t bindingIndex) noexcept;
	static VkShaderStageFlags ToVkShaderStages(ShaderStageMask visibilityMask) noexcept;
	VkBuffer ResolveBuffer(RhiGpuVirtualAddress gpuAddress) const noexcept;
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
	void FlushGraphicsDescriptorSets() noexcept;
	void FlushComputeDescriptorSets() noexcept;

	static constexpr std::uint32_t MaxRenderTargets = 8;

	const VulkanRhi* m_rhi = nullptr;
	const VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanDescriptorAllocator* m_descriptorAllocator = nullptr;
	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	const VulkanBindingLayout* m_graphicsBindingLayout = nullptr;
	const VulkanBindingLayout* m_computeBindingLayout = nullptr;
	VkPipelineLayout m_graphicsPipelineLayout = VK_NULL_HANDLE;
	VkPipelineLayout m_computePipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet> m_graphicsDescriptorSets;
	std::vector<VkDescriptorSet> m_computeDescriptorSets;
	std::vector<bool> m_graphicsDirtyDescriptorSets;
	std::vector<bool> m_computeDirtyDescriptorSets;
	std::vector<bool> m_graphicsBoundDescriptorSets;
	std::vector<bool> m_computeBoundDescriptorSets;
	std::vector<RhiDescriptorTableBinding> m_retainedDescriptorTables;
	std::vector<RhiGpuDescriptorHandle> m_retainedDescriptorHandles;
	std::vector<VkBuffer> m_retainedDescriptorBuffers;
	VulkanDebugEventFunctions m_debugEvents = {};
	std::array<VkImageView, MaxRenderTargets> m_renderTargets = {};
	std::uint32_t m_renderTargetCount = 0;
	VkImageView m_depthStencil = VK_NULL_HANDLE;
	VkRect2D m_scissorRect = {};
	bool m_hasScissorRect = false;
	bool m_dynamicRenderingActive = false;
};
