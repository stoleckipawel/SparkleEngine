#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "Meshes/GPUMesh.h"

#include <memory>
#include <cstdint>
#include <unordered_map>

class Mesh;
class RenderHardwareInterface;

class SPARKLE_RENDERER_API GPUMeshCache final
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
	struct CacheEntry final
	{
		std::unique_ptr<GPUMesh> Mesh;
		std::uint64_t GeometryRevision = 0;
	};

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::unordered_map<const Mesh*, CacheEntry> m_cache;
};
