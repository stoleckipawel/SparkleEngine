#pragma once

#include "Renderer/Public/RendererAPI.h"
#include "GPU/GPUMesh.h"

#include <memory>
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

  private:
	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::unordered_map<const Mesh*, std::unique_ptr<GPUMesh>> m_cache;
};
