#pragma once

#include <memory>

struct Frustum;
class GpuMeshCache;
class MaterialCache;
class RenderWorld;
class TaskExecutor;
class TextureCache;
struct RenderSceneDynamicData;
struct RenderSceneData;

class RenderPreparationGraph final
{
public:
	RenderPreparationGraph(
	    TaskExecutor& taskExecutor,
	    MaterialCache& materialCache,
	    GpuMeshCache& gpuMeshCache,
	    TextureCache& textureCache);
	~RenderPreparationGraph() noexcept;

	RenderPreparationGraph(const RenderPreparationGraph&) = delete;
	RenderPreparationGraph& operator=(const RenderPreparationGraph&) = delete;
	RenderPreparationGraph(RenderPreparationGraph&&) = delete;
	RenderPreparationGraph& operator=(RenderPreparationGraph&&) = delete;

	void Execute(const RenderWorld& world, const RenderSceneDynamicData& dynamic, const Frustum& frustum, RenderSceneData& output);
	void ResetHistory() noexcept;

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
