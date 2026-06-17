#pragma once

#include "Renderer/Public/RendererAPI.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cstdint>
#include <string_view>

class SPARKLE_RENDERER_API RenderCommandContext final
{
  public:
	explicit RenderCommandContext(RenderCommandList& commandList) noexcept;
	~RenderCommandContext() noexcept = default;

	RenderCommandContext(const RenderCommandContext&) = delete;
	RenderCommandContext& operator=(const RenderCommandContext&) = delete;
	RenderCommandContext(RenderCommandContext&&) = delete;
	RenderCommandContext& operator=(RenderCommandContext&&) = delete;

	void EnableDrawDispatchDiagnostics() noexcept;
	bool IsDrawDispatchDiagnosticsEnabled() const noexcept { return m_drawDispatchDiagnosticsEnabled; }

	void SetPipelineState(const RenderPipelineState& pipelineState) noexcept;
	void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept;
	void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept;

	void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept;

	void BindVertexBuffer(const RhiVertexBufferView& view) noexcept;

	void BindIndexBuffer(const RhiIndexBufferView& view) noexcept;

	void BindConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;
	void SetPushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept;
	void BindShaderResourceAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;
	void BindUnorderedAccessAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;

	void BindDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept;
	void BindDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept;
	void BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept;
	void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept;
	void SetComputePushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept;
	void BindComputeShaderResourceAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;
	void BindComputeUnorderedAccessAddress(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept;

	void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept;

	void SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept;

	void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept;

	void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil = 0) noexcept;
	void SetViewport(const RhiViewport& viewport) noexcept;

	void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) noexcept;
	void SetScissorRect(const RhiRect& scissorRect) noexcept;

	void SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept;

	void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept;

	void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept;

	void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept;
	void BuildBottomLevelAccelerationStructure(
	    const RhiRayTracingGeometryDesc& geometry,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress) noexcept;
	void BuildTopLevelAccelerationStructure(
	    RhiGpuVirtualAddress instanceDescsGpuAddress,
	    std::uint32_t instanceCount,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress,
	    ERhiClassicTlasBuildMode buildMode = ERhiClassicTlasBuildMode::Build) noexcept;
	void BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc& desc) noexcept;
	bool SupportsDiagnosticScopes() const noexcept;
	void BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept;
	void EndDiagnosticScope() noexcept;
	void InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept;

	void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept;

	void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept;
	void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept;
	void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept;

	RenderCommandList& GetRenderCommandList() const noexcept { return *m_commandList; }

  private:
	void EmitDrawMarker() noexcept;
	void EmitDispatchMarker(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept;

	RenderCommandList* m_commandList = nullptr;
	bool m_drawDispatchDiagnosticsEnabled = false;
	std::uint32_t m_drawCount = 0;
	std::uint32_t m_dispatchCount = 0;
};
