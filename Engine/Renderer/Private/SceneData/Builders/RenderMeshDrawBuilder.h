#pragma once

#include "Rendering/RenderObjectId.h"
#include "SceneData/Builders/RenderMeshMorphBuilder.h"

#include <DirectXMath.h>
#include <map>
#include <vector>

class GPUMeshCache;
class RenderWorld;
struct RenderFrameDynamicData;
struct RenderSceneData;
struct MeshRenderItem;

// Converts immutable render proxies and per-frame object state into GPU-ready mesh draws.
// This capability exclusively owns mesh temporal history; scene orchestration does not.
class RenderMeshDrawBuilder final
{
  public:
	explicit RenderMeshDrawBuilder(GPUMeshCache& gpuMeshCache) noexcept;

	void Build(const RenderWorld& world, const RenderFrameDynamicData& dynamic, RenderSceneData& sceneData);
	void ResetHistory() noexcept;

  private:
	void AppendSkinningData(const RenderFrameDynamicData& dynamic, RenderSceneData& sceneData,
	                        std::map<RenderObjectId, std::uint32_t>& outJointMatrixOffsets);
	void AppendVisibleMeshItems(
	    const RenderWorld& world,
	    const RenderFrameDynamicData& dynamic,
	    const std::map<RenderObjectId, std::uint32_t>& jointMatrixOffsets,
	    const RenderSceneData& sceneData,
	    std::vector<MeshRenderItem>& outItems,
	    std::map<RenderObjectId, DirectX::XMFLOAT4X4>& outCurrentWorldMatrices);
	void BuildBatches(const RenderWorld& world, std::vector<MeshRenderItem> items, RenderSceneData& sceneData) const;
	void PublishWorkload(RenderSceneData& sceneData) const;

	GPUMeshCache& m_gpuMeshCache;
	std::map<RenderObjectId, DirectX::XMFLOAT4X4> m_previousWorldMatrices;
	std::map<RenderObjectId, std::vector<DirectX::XMFLOAT4X4>> m_previousSkinningMatrices;
	std::map<RenderObjectId, std::vector<DirectX::XMFLOAT4X4>> m_currentSkinningMatrices;
	RenderMeshMorphBuilder m_morphBuilder;
};
