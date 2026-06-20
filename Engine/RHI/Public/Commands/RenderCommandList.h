#pragma once

#include "../Core/RhiBackendApi.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Diagnostics/RhiDiagnostics.h"
#include "../Interop/ResourceState.h"
#include "../Interop/RhiNativeHandles.h"
#include "../Pipeline/RhiPipelineStateDesc.h"
#include "../RayTracing/RhiRayTracingDesc.h"
#include "../Resources/RhiResourceDesc.h"
#include "../RHIAPI.h"

#include <cstdint>
#include <string_view>

class SPARKLE_RHI_API RenderCommandList
{
  public:
	virtual ~RenderCommandList() noexcept = default;

	virtual ERhiBackendApi GetBackendApi() const noexcept = 0;
	virtual NativeGraphicsCommandListHandle GetNativeHandle(const struct RhiNativeInteropRequest& request) const noexcept = 0;
	virtual bool SupportsDiagnosticScopes() const noexcept = 0;
	virtual void BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept = 0;
	virtual void EndDiagnosticScope() noexcept = 0;
	virtual void InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color = {}) noexcept = 0;
	virtual void SetPipelineState(const RenderPipelineState& pipelineState) noexcept = 0;
	virtual void SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept = 0;
	virtual void SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept = 0;
	virtual void BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept = 0;
	virtual void BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept = 0;
	virtual void SetGraphicsPushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept = 0;
	virtual void BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept = 0;
	virtual void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept = 0;
	virtual void BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept = 0;
	virtual void SetComputePushConstants(
	    std::uint32_t bindingIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept = 0;
	virtual void SetPrimitiveTopology(RhiPrimitiveTopology topology) noexcept = 0;
	virtual void BindVertexBuffer(const RhiVertexBufferView& view) noexcept = 0;
	virtual void BindIndexBuffer(const RhiIndexBufferView& view) noexcept = 0;
	virtual void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept = 0;
	virtual void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const RhiCpuDescriptorHandle* rtvs,
	    const RhiCpuDescriptorHandle* dsv = nullptr) noexcept = 0;
	virtual void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept = 0;
	virtual void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil = 0) noexcept = 0;
	virtual void SetViewport(const RhiViewport& viewport) noexcept = 0;
	virtual void SetScissorRect(const RhiRect& rect) noexcept = 0;
	virtual void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept = 0;
	virtual void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept = 0;
	virtual void Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept = 0;
	virtual void BuildBottomLevelAccelerationStructure(
	    const RhiRayTracingGeometryDesc& geometry,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress) noexcept = 0;
	virtual void BuildTopLevelAccelerationStructure(
	    RhiGpuVirtualAddress instanceDescsGpuAddress,
	    std::uint32_t instanceCount,
	    RhiGpuVirtualAddress scratchGpuAddress,
	    RhiGpuVirtualAddress resultGpuAddress,
	    ERhiClassicTlasBuildMode buildMode = ERhiClassicTlasBuildMode::Build) noexcept = 0;
	virtual void BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc&) noexcept {}
	virtual void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept = 0;
	virtual void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept = 0;
	virtual void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept = 0;
	virtual void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept = 0;
};
