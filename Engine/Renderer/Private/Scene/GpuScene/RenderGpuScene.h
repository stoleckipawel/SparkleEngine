#pragma once

#include <cstdint>
#include <memory>

class GpuMeshCache;
class RhiResourceService;
struct PreparedRenderScene;
struct RenderView;
struct RenderSceneGpuBindings;

class RenderGpuScene final
{
public:
	RenderGpuScene(RhiResourceService& resourceService, const GpuMeshCache& meshes);
	~RenderGpuScene() noexcept;

	RenderGpuScene(const RenderGpuScene&) = delete;
	RenderGpuScene& operator=(const RenderGpuScene&) = delete;

	const RenderSceneGpuBindings& Update(const PreparedRenderScene& preparedScene, const RenderView& view, std::uint32_t frameIndex);
	void Reset() noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
