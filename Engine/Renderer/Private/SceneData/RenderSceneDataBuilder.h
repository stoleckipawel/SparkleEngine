#pragma once

#include "SceneData/RenderSceneSnapshot.h"

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
	void BuildMeshDraws(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const;
	void BuildLighting(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData) const noexcept;

	MaterialCacheManager* m_materialCache = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
};
