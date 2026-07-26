#pragma once

#include "Rendering/RenderWorldDelta.h"

#include <map>
#include <memory>
#include <string>

class GpuSceneSlotAllocator;
class RhiCommandSubmissionService;

struct RenderProxy final
{
	RenderObjectId Object;
	RenderObjectStaticData Static;
	std::uint32_t GpuSceneSlot = 0;
};

enum class RenderWorldApplyStatus : std::uint8_t { Applied, Duplicate, Stale, OutOfOrder, Rejected };

class RenderWorld final
{
  public:
	explicit RenderWorld(
	    RhiCommandSubmissionService* submissionService = nullptr);
	~RenderWorld() noexcept;

	RenderWorldApplyStatus Apply(const RenderWorldDelta& delta, std::string& diagnostic);
	RenderWorldApplyStatus Validate(const RenderWorldDelta& delta, std::string& diagnostic) const;
	const RenderProxy* Find(RenderObjectId object) const noexcept;
	const std::map<RenderObjectId, RenderProxy>& GetProxies() const noexcept { return m_proxies; }
	const RenderMaterialTable& GetMaterials() const noexcept { return m_materials; }
	const RenderTextureTable& GetTextures() const noexcept { return m_textures; }
	const std::optional<SceneSkyDesc>& GetSky() const noexcept { return m_sky; }
	const std::vector<RenderMeshInstanceGroupData>& GetInstanceGroups() const noexcept { return m_instanceGroups; }
	std::uint64_t GetSceneGeneration() const noexcept { return m_sceneGeneration; }
	std::uint64_t GetSequenceNumber() const noexcept { return m_sequenceNumber; }
	std::uint64_t GetStructuralRevision() const noexcept { return m_structuralRevision; }
	std::uint64_t GetMaterialRevision() const noexcept { return m_materialRevision; }
	bool ConsumeHistoryReset() noexcept;

  private:
	void ApplyDestroys(const RenderWorldDelta& delta);
	void ApplyCreates(const RenderWorldDelta& delta);
	void ApplyUpdates(const RenderWorldDelta& delta);
	void PublishResources(const RenderWorldDelta& delta);
	std::map<RenderObjectId, RenderProxy> m_proxies;
	std::unique_ptr<GpuSceneSlotAllocator> m_gpuSceneSlots;
	RenderMaterialTable m_materials;
	RenderTextureTable m_textures;
	std::optional<SceneSkyDesc> m_sky;
	std::vector<RenderMeshInstanceGroupData> m_instanceGroups;
	std::uint64_t m_sceneGeneration = 0;
	std::uint64_t m_sequenceNumber = 0;
	std::uint64_t m_structuralRevision = 0;
	std::uint64_t m_materialRevision = 0;
	bool m_historyReset = false;
};
