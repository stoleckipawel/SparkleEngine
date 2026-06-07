#pragma once

#include "SceneData/Lifecycle/RenderSceneSnapshot.h"

class GPUMeshCache;
class MaterialCacheManager;
struct RenderSceneData;

class RenderSceneDataBuilder final
{
  public:
	RenderSceneDataBuilder(MaterialCacheManager& materialCache, GPUMeshCache& gpuMeshCache) noexcept;
	~RenderSceneDataBuilder() noexcept = default;

	RenderSceneDataBuilder(const RenderSceneDataBuilder&) = delete;
	RenderSceneDataBuilder& operator=(const RenderSceneDataBuilder&) = delete;
	RenderSceneDataBuilder(RenderSceneDataBuilder&&) = delete;
	RenderSceneDataBuilder& operator=(RenderSceneDataBuilder&&) = delete;

	RenderSceneData Build(const RenderSceneSnapshot& sceneSnapshot);

 private:
	void BuildMaterials(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const;
	void BuildMeshInstanceBatches(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const;

	MaterialCacheManager* m_materialCache = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
};
