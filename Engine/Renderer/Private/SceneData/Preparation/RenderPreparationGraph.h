#pragma once

#include <DirectXMath.h>

#include <memory>

struct Frustum;
class GpuMeshCache;
class RenderScene;
class TaskExecutor;
class TextureCache;
struct RenderSceneData;

class RenderPreparationGraph final
{
public:
	RenderPreparationGraph(TaskExecutor& taskExecutor, GpuMeshCache& gpuMeshCache, TextureCache& textureCache);
	~RenderPreparationGraph() noexcept;

	RenderPreparationGraph(const RenderPreparationGraph&) = delete;
	RenderPreparationGraph& operator=(const RenderPreparationGraph&) = delete;
	RenderPreparationGraph(RenderPreparationGraph&&) = delete;
	RenderPreparationGraph& operator=(RenderPreparationGraph&&) = delete;

	void Execute(RenderScene& scene, const Frustum& frustum, const DirectX::XMFLOAT3& cameraPosition, RenderSceneData& output);

private:
	struct Impl;
	std::unique_ptr<Impl> m_impl;
};
