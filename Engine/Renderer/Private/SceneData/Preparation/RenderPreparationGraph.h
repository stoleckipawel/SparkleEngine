#pragma once

#include <memory>

struct Frustum;
class GPUMeshCache;
class MaterialCacheManager;
class RenderWorld;
class TaskExecutor;
class TextureManager;
struct RenderFrameDynamicData;
struct RenderSceneData;

class RenderPreparationGraph final
{
  public:
	RenderPreparationGraph(
	    TaskExecutor& taskExecutor,
	    MaterialCacheManager& materialCache,
	    GPUMeshCache& gpuMeshCache,
	    TextureManager& textureManager);
	~RenderPreparationGraph() noexcept;

	RenderPreparationGraph(
	    const RenderPreparationGraph&) = delete;
	RenderPreparationGraph& operator=(
	    const RenderPreparationGraph&) = delete;
	RenderPreparationGraph(
	    RenderPreparationGraph&&) = delete;
	RenderPreparationGraph& operator=(
	    RenderPreparationGraph&&) = delete;

	RenderSceneData Execute(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    const Frustum& frustum);
	void ResetHistory() noexcept;

  private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
