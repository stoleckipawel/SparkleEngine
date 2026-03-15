#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

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

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) noexcept;

	void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view) noexcept;

	void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view) noexcept;

	void BindConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;

	void BindDescriptorTable(std::uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) noexcept;

	void SetDescriptorHeaps(std::uint32_t heapCount, ID3D12DescriptorHeap* const* heaps) noexcept;

	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr) noexcept;

	void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
	    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr) noexcept;

	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) noexcept;

	void ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, std::uint8_t stencil = 0) noexcept;

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

	void CopyResource(ID3D12Resource* destinationResource, ID3D12Resource* sourceResource) noexcept;

	void AliasResource(ID3D12Resource* beforeResource, ID3D12Resource* afterResource) noexcept;
	void TransitionResource(ID3D12Resource* resource, ResourceState before, ResourceState after) noexcept;

	ID3D12GraphicsCommandList* GetCommandList() const noexcept { return m_cmdList; }

  private:
	static D3D12_RESOURCE_STATES MapToD3D12State(ResourceState state) noexcept;

	ID3D12GraphicsCommandList* m_cmdList = nullptr;
};