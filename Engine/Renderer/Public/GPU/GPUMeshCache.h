// =============================================================================
// GPUMeshCache.h — Lazy GPU mesh upload manager
// =============================================================================
//
// Caches GPU meshes by their CPU mesh pointer. Uploads on first access.
// Owned by Renderer — provides lazy upload for render passes.
//
// USAGE:
//   GPUMeshCache cache(rhi);
//   GPUMesh* gpuMesh = cache.GetOrUpload(cpuMesh);
//   gpuMesh->Bind(cmdList);
//
// OWNERSHIP:
//   - Renderer owns GPUMeshCache
//   - GPUMeshCache owns GPUMesh instances
//   - CPU Mesh lifetime must exceed cache usage
//
// =============================================================================

#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/GPU/GPUMesh.h"

#include <memory>
#include <unordered_map>

class D3D12Rhi;
class Mesh;

// =============================================================================
// GPUMeshCache
// =============================================================================

class SPARKLE_RENDERER_API GPUMeshCache final
{
  public:
	explicit GPUMeshCache(D3D12Rhi& rhi) noexcept;
	~GPUMeshCache() = default;

	GPUMeshCache(const GPUMeshCache&) = delete;
	GPUMeshCache& operator=(const GPUMeshCache&) = delete;
	GPUMeshCache(GPUMeshCache&&) noexcept = default;
	GPUMeshCache& operator=(GPUMeshCache&&) noexcept = default;

	// -------------------------------------------------------------------------
	// Cache Operations
	// -------------------------------------------------------------------------

	// Returns cached GPUMesh or uploads new one. Never returns null on success.
	// Returns nullptr if upload fails.
	GPUMesh* GetOrUpload(const Mesh& cpuMesh);

	// Releases all cached GPU meshes
	void Clear() noexcept;

	// -------------------------------------------------------------------------
	// Queries
	// -------------------------------------------------------------------------

	std::size_t GetCachedCount() const noexcept { return m_cache.size(); }
	bool Contains(const Mesh& cpuMesh) const noexcept;

  private:
	D3D12Rhi* m_rhi;
	std::unordered_map<const Mesh*, std::unique_ptr<GPUMesh>> m_cache;
};
