#pragma once

#include "SceneData/Lifecycle/RenderSceneSnapshot.h"

#include <DirectXMath.h>
#include <vector>

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
	void BuildMeshInstanceBatches(const RenderSceneSnapshot& sceneSnapshot, RenderSceneData& sceneData);

	MaterialCacheManager* m_materialCache = nullptr;
	GPUMeshCache* m_gpuMeshCache = nullptr;
	std::vector<DirectX::XMFLOAT4X4> m_previousMeshWorldMatrices;
	std::vector<DirectX::XMFLOAT4X4> m_previousJointMatrices;
};
