#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Renderer/Public/GPU/GPUMesh.h"

#include <memory>
#include <unordered_map>

class D3D12Rhi;
class Mesh;

class SPARKLE_RENDERER_API GPUMeshCache final
{
  public:
	explicit GPUMeshCache(D3D12Rhi& rhi) noexcept;
	~GPUMeshCache() = default;

	GPUMeshCache(const GPUMeshCache&) = delete;
	GPUMeshCache& operator=(const GPUMeshCache&) = delete;
	GPUMeshCache(GPUMeshCache&&) noexcept = default;
	GPUMeshCache& operator=(GPUMeshCache&&) noexcept = default;

	GPUMesh* GetOrUpload(const Mesh& cpuMesh);

	void Clear() noexcept;

	std::size_t GetCachedCount() const noexcept { return m_cache.size(); }
	bool Contains(const Mesh& cpuMesh) const noexcept;

  private:
	D3D12Rhi* m_rhi;
	std::unordered_map<const Mesh*, std::unique_ptr<GPUMesh>> m_cache;
};
