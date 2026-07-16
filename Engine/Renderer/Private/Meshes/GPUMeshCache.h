#pragma once

#include "Meshes/GPUMesh.h"

#include <cstdint>
#include <memory>
#include <unordered_map>

class Mesh;
class RenderHardwareInterface;

class GPUMeshCache final
{
  public:
	explicit GPUMeshCache(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~GPUMeshCache() = default;

	GPUMeshCache(const GPUMeshCache&) = delete;
	GPUMeshCache& operator=(const GPUMeshCache&) = delete;
	GPUMeshCache(GPUMeshCache&&) noexcept = default;
	GPUMeshCache& operator=(GPUMeshCache&&) noexcept = default;

	GPUMesh* GetOrUpload(const Mesh& cpuMesh);

	void Clear() noexcept;

	std::size_t GetCachedCount() const noexcept { return m_cache.size(); }
	bool Contains(const Mesh& cpuMesh) const noexcept;
	const GPUMesh* Find(const Mesh& cpuMesh) const noexcept;

  private:
	GpuMeshHandle AllocateHandle() noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::uint64_t m_nextGpuMeshHandle = 1u;
	std::unordered_map<const Mesh*, std::unique_ptr<GPUMesh>> m_cache;
};
