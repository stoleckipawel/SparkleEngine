#pragma once

#include "Meshes/GPUMesh.h"

#include <cstdint>
#include <map>
#include <memory>
#include <unordered_map>

class ImmutableRenderMeshHandle;
class Mesh;
class RenderHardwareInterface;

class GPUMeshCache final
{
  public:
	explicit GPUMeshCache(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~GPUMeshCache() noexcept;

	GPUMeshCache(const GPUMeshCache&) = delete;
	GPUMeshCache& operator=(const GPUMeshCache&) = delete;
	GPUMeshCache(GPUMeshCache&&) = delete;
	GPUMeshCache& operator=(GPUMeshCache&&) = delete;

	GPUMesh* GetOrUpload(const ImmutableRenderMeshHandle& mesh);
	const GPUMesh* Resolve(GpuMeshHandle handle) const noexcept;

	void Clear() noexcept;

	std::size_t GetCachedCount() const noexcept { return m_cache.size(); }
	bool Contains(const Mesh& cpuMesh) const noexcept;
	const GPUMesh* Find(const Mesh& cpuMesh) const noexcept;

  private:
	GpuMeshHandle AllocateHandle() noexcept;
	using CacheKey = std::pair<std::uint64_t, std::uint32_t>;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::uint64_t m_nextGpuMeshHandle = 1u;
	std::map<CacheKey, std::unique_ptr<GPUMesh>> m_cache;
	std::unordered_map<std::uint64_t, const GPUMesh*> m_handles;
	std::unordered_map<const Mesh*, GpuMeshHandle> m_sourceHandles;
};
