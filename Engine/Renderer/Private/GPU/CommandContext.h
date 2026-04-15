#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"

#include <d3d12.h>
#include <cstdint>

class D3D12PipelineState;
class D3D12RootSignature;

class SPARKLE_RENDERER_API CommandContext final
{
  public:
	explicit CommandContext(ID3D12GraphicsCommandList* cmdList) noexcept;
	~CommandContext() noexcept = default;

	CommandContext(const CommandContext&) = delete;
	CommandContext& operator=(const CommandContext&) = delete;
	CommandContext(CommandContext&&) = delete;
	CommandContext& operator=(CommandContext&&) = delete;

	void SetPipelineState(ID3D12PipelineState* pso) noexcept;

	void SetRootSignature(ID3D12RootSignature* rootSig) noexcept;
	void SetComputeRootSignature(ID3D12RootSignature* rootSig) noexcept;

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) noexcept;

	void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view) noexcept;

	void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view) noexcept;

	void BindConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;
	void SetRoot32BitConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept;
	void BindRootShaderResourceView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;
	void BindRootUnorderedAccessView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;

	void BindDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept;
	void BindComputeRootConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;
	void BindComputeDescriptorTable(std::uint32_t rootParameterIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept;
	void SetComputeRoot32BitConstants(
	    std::uint32_t rootParameterIndex,
	    std::uint32_t num32BitValues,
	    const void* data,
	    std::uint32_t destOffsetIn32BitValues) noexcept;
	void BindComputeRootShaderResourceView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;
	void BindComputeRootUnorderedAccessView(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;

	void SetDescriptorHeaps(std::uint32_t heapCount, ID3D12DescriptorHeap* const* heaps) noexcept;

	void SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv = nullptr) noexcept;

	void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const RhiCpuDescriptorHandle* rtvs,
	    const RhiCpuDescriptorHandle* dsv = nullptr) noexcept;

	void ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept;

	void ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil = 0) noexcept;

	void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) noexcept;

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

	void CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept;

	void AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept;
	void TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept;
	void UnorderedAccessBarrier(NativeResourceHandle resource) noexcept;

	ID3D12GraphicsCommandList* GetCommandList() const noexcept { return m_cmdList; }

  private:
	static D3D12_RESOURCE_STATES MapToD3D12State(ResourceState state) noexcept;

	ID3D12GraphicsCommandList* m_cmdList = nullptr;
};