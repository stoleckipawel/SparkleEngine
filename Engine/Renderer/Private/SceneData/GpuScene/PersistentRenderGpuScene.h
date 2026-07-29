#pragma once

#include <cstdint>
#include <memory>

class GpuMeshCache;
class RhiResourceService;
struct RenderSceneData;
struct RenderSceneGpuData;

class PersistentRenderGpuScene final
{
  public:
	PersistentRenderGpuScene(
	    RhiResourceService& resourceService,
	    const GpuMeshCache& meshes);
	~PersistentRenderGpuScene() noexcept;

	PersistentRenderGpuScene(const PersistentRenderGpuScene&) = delete;
	PersistentRenderGpuScene& operator=(const PersistentRenderGpuScene&) = delete;

	const RenderSceneGpuData& Update(
	    const RenderSceneData& sceneData,
	    std::uint32_t frameIndex);
	void Reset() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
