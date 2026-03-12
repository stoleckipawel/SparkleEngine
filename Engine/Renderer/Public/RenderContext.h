// ============================================================================
// RenderContext.h
// ----------------------------------------------------------------------------
// High-level command abstraction for render passes (Frostbite-inspired).
//
// PURPOSE:
//   Wraps D3D12 command list operations with semantic methods, providing a
//   clean API for render passes. Centralizes barrier management and creates
//   an abstraction boundary for future multi-backend support.
//
#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

#include <d3d12.h>
#include <cstdint>

// Forward declarations
class D3D12PipelineState;
class D3D12RootSignature;

// ============================================================================
// RenderContext
// ============================================================================

/// High-level command recording interface for render passes.
/// Wraps D3D12 command list with semantic operations.
class SPARKLE_RENDERER_API RenderContext final
{
  public:
	// -------------------------------------------------------------------------
	// Construction
	// -------------------------------------------------------------------------

	/// @param cmdList Active command list (must be in recording state)
	explicit RenderContext(ID3D12GraphicsCommandList* cmdList) noexcept;
	~RenderContext() noexcept = default;

	RenderContext(const RenderContext&) = delete;
	RenderContext& operator=(const RenderContext&) = delete;
	RenderContext(RenderContext&&) = delete;
	RenderContext& operator=(RenderContext&&) = delete;

	// -------------------------------------------------------------------------
	// Pipeline State
	// -------------------------------------------------------------------------

	void SetPipelineState(ID3D12PipelineState* pso) noexcept;

	void SetRootSignature(ID3D12RootSignature* rootSig) noexcept;

	// -------------------------------------------------------------------------
	// Geometry / Input Assembly
	// -------------------------------------------------------------------------

	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology) noexcept;

	void BindVertexBuffer(const D3D12_VERTEX_BUFFER_VIEW& view) noexcept;

	void BindIndexBuffer(const D3D12_INDEX_BUFFER_VIEW& view) noexcept;

	// -------------------------------------------------------------------------
	// Resource Binding
	// -------------------------------------------------------------------------

	/// @param rootParameterIndex Root parameter index
	/// @param gpuAddress GPU virtual address of the constant buffer
	void BindConstantBuffer(std::uint32_t rootParameterIndex, D3D12_GPU_VIRTUAL_ADDRESS gpuAddress) noexcept;

	/// @param rootParameterIndex Root parameter index
	/// @param baseDescriptor Base GPU descriptor handle for the table
	void BindDescriptorTable(std::uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor) noexcept;

	// -------------------------------------------------------------------------
	// Render Targets
	// -------------------------------------------------------------------------

	/// @param rtv Render target view handle
	/// @param dsv Depth-stencil view handle (optional, use nullptr/empty if not needed)
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr) noexcept;

	/// @param numRTVs Number of render targets
	/// @param rtvs Array of render target view handles
	/// @param dsv Depth-stencil view handle (optional)
	void SetRenderTargets(
	    std::uint32_t numRTVs,
	    const D3D12_CPU_DESCRIPTOR_HANDLE* rtvs,
	    const D3D12_CPU_DESCRIPTOR_HANDLE* dsv = nullptr) noexcept;

	/// @param rtv Render target view to clear
	/// @param color RGBA clear color (array of 4 floats)
	void ClearRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float color[4]) noexcept;

	/// @param dsv Depth-stencil view to clear
	/// @param depth Depth clear value (typically 1.0 or 0.0 for reversed-Z)
	/// @param stencil Stencil clear value (typically 0)
	void ClearDepthStencil(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth, std::uint8_t stencil = 0) noexcept;

	// -------------------------------------------------------------------------
	// Viewport & Scissor
	// -------------------------------------------------------------------------

	void SetViewport(float x, float y, float width, float height, float minDepth = 0.0f, float maxDepth = 1.0f) noexcept;

	void SetScissorRect(std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom) noexcept;

	// -------------------------------------------------------------------------
	// Draw Commands
	// -------------------------------------------------------------------------

	/// Draws indexed, instanced primitives.
	void DrawIndexedInstanced(
	    std::uint32_t indexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startIndexLocation,
	    std::int32_t baseVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept;

	/// Draws non-indexed, instanced primitives.
	void DrawInstanced(
	    std::uint32_t vertexCountPerInstance,
	    std::uint32_t instanceCount,
	    std::uint32_t startVertexLocation,
	    std::uint32_t startInstanceLocation) noexcept;

	// -------------------------------------------------------------------------
	// Resource Barriers
	// -------------------------------------------------------------------------

	/// Transitions a resource between states.
	/// @param resource The D3D12 resource to transition
	/// @param before Current state (use ResourceState enum)
	/// @param after Target state (use ResourceState enum)
	void TransitionResource(ID3D12Resource* resource, ResourceState before, ResourceState after) noexcept;

	// -------------------------------------------------------------------------
	// Native Access (Escape Hatch)
	// -------------------------------------------------------------------------

	/// Use sparingly - prefer semantic methods above.
	ID3D12GraphicsCommandList* GetNativeCommandList() const noexcept { return m_cmdList; }

  private:
	/// Maps ResourceState enum to D3D12_RESOURCE_STATES.
	static D3D12_RESOURCE_STATES MapToD3D12State(ResourceState state) noexcept;

	ID3D12GraphicsCommandList* m_cmdList = nullptr;
};
