#pragma once

#include "Commands/RenderCommandList.h"
#include "Vulkan/VulkanIncludes.h"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

class VulkanRenderCommandList final : public RenderCommandList
{
  public:
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
	void SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept override;
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
	static VkImageView DecodeImageViewHandle(RhiCpuDescriptorHandle handle) noexcept;
	static VkDebugUtilsLabelEXT BuildLabel(const char* label, RhiDiagnosticLabelColor color) noexcept;

	static constexpr std::uint32_t MaxRenderTargets = 8;

	VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
	PFN_vkCmdBeginDebugUtilsLabelEXT m_beginDebugUtilsLabel = nullptr;
	PFN_vkCmdEndDebugUtilsLabelEXT m_endDebugUtilsLabel = nullptr;
	PFN_vkCmdInsertDebugUtilsLabelEXT m_insertDebugUtilsLabel = nullptr;
	std::array<VkImageView, MaxRenderTargets> m_renderTargets = {};
	std::uint32_t m_renderTargetCount = 0;
	VkRect2D m_scissorRect = {};
	bool m_hasScissorRect = false;
};