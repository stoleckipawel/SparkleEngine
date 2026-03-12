// =============================================================================
// GPUMesh.h — GPU-resident mesh buffers for rendering
// =============================================================================
//
// Owns D3D12 vertex and index buffers uploaded from CPU MeshData.
// Created and cached by GPUMeshCache — not directly instantiated by user code.
//
// USAGE:
//   GPUMesh gpuMesh;
//   gpuMesh.Upload(rhi, meshData);
//   gpuMesh.Bind(cmdList);
//   cmdList->DrawIndexedInstanced(gpuMesh.GetIndexCount(), 1, 0, 0, 0);
//
// OWNERSHIP:
//   - GPUMeshCache owns GPUMesh instances
//   - Renderer owns GPUMeshCache
//   - GameFramework has no knowledge of this class
//
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

class D3D12Rhi;
struct MeshData;

// =============================================================================
// GPUMesh
// =============================================================================

class SPARKLE_RENDERER_API GPUMesh final
{
  public:
	GPUMesh() = default;
	~GPUMesh() = default;

	GPUMesh(const GPUMesh&) = delete;
	GPUMesh& operator=(const GPUMesh&) = delete;
	GPUMesh(GPUMesh&&) noexcept = default;
	GPUMesh& operator=(GPUMesh&&) noexcept = default;

	// -------------------------------------------------------------------------
	// Upload
	// -------------------------------------------------------------------------

	// Creates GPU buffers from CPU mesh data. Call once per mesh.
	// Returns true on success.
	bool Upload(D3D12Rhi& rhi, const MeshData& meshData);

	// -------------------------------------------------------------------------
	// Binding
	// -------------------------------------------------------------------------

	// Sets vertex and index buffers on the command list (IA stage)
	void Bind(ID3D12GraphicsCommandList* cmdList) const noexcept;

	// -------------------------------------------------------------------------
	// Accessors
	// -------------------------------------------------------------------------

	std::uint32_t GetIndexCount() const noexcept { return m_indexCount; }
	std::uint32_t GetVertexCount() const noexcept { return m_vertexCount; }

	bool IsValid() const noexcept { return m_vertexBuffer && m_indexBuffer; }

	const D3D12_VERTEX_BUFFER_VIEW& GetVertexBufferView() const noexcept { return m_vertexBufferView; }
	const D3D12_INDEX_BUFFER_VIEW& GetIndexBufferView() const noexcept { return m_indexBufferView; }

  private:
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource2> m_indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView{};

	std::uint32_t m_vertexCount = 0;
	std::uint32_t m_indexCount = 0;
};
