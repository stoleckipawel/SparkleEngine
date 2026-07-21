#pragma once

#include "Rendering/RenderWorldDelta.h"

#include <map>
#include <string>
#include <utility>

struct RenderProxy final
{
	RenderObjectId Object;
	ImmutableRenderMeshHandle Mesh;
	MaterialHandle Material = MaterialHandle::Invalid();
	Assets::CookedAssetId SkeletonAssetId = Assets::InvalidCookedAssetId;
	SceneMeshKind MeshKind = SceneMeshKind::Static;
	SceneMeshAssetIndex MeshAssetIndex = kInvalidSceneMeshAssetIndex;
	SceneMeshInstanceGroupIndex InstanceGroupIndex = kInvalidSceneMeshInstanceGroupIndex;
};

enum class RenderWorldApplyStatus : std::uint8_t { Applied, Duplicate, Stale, OutOfOrder, Rejected };

class RenderWorld final
{
  public:
	RenderWorldApplyStatus Apply(const RenderWorldDelta& delta, std::string& diagnostic);
	const RenderProxy* Find(RenderObjectId object) const noexcept;
	const std::map<RenderObjectId, RenderProxy>& GetProxies() const noexcept { return m_proxies; }
	const RenderMaterialTable& GetMaterials() const noexcept { return m_materials; }
	const RenderTextureTable& GetTextures() const noexcept { return m_textures; }
	const std::optional<SceneSkyDesc>& GetSky() const noexcept { return m_sky; }
	const std::vector<RenderMeshInstanceGroupData>& GetInstanceGroups() const noexcept { return m_instanceGroups; }
	std::uint64_t GetSceneGeneration() const noexcept { return m_sceneGeneration; }
	std::uint64_t GetSequenceNumber() const noexcept { return m_sequenceNumber; }
	bool ConsumeHistoryReset() noexcept { return std::exchange(m_historyReset, false); }

  private:
	RenderWorldApplyStatus ValidateSequence(const RenderWorldDelta& delta, std::string& diagnostic) const;
	bool ValidateOperations(const RenderWorldDelta& delta, std::string& diagnostic) const;
	void ApplyDestroys(const RenderWorldDelta& delta);
	void ApplyCreates(const RenderWorldDelta& delta);
	void ApplyUpdates(const RenderWorldDelta& delta);
	void PublishResources(const RenderWorldDelta& delta);
	std::map<RenderObjectId, RenderProxy> m_proxies;
	RenderMaterialTable m_materials;
	RenderTextureTable m_textures;
	std::optional<SceneSkyDesc> m_sky;
	std::vector<RenderMeshInstanceGroupData> m_instanceGroups;
	std::uint64_t m_sceneGeneration = 0;
	std::uint64_t m_sequenceNumber = 0;
	bool m_historyReset = false;
};
