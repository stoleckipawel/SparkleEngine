#pragma once

#include "Meshes/GPUMesh.h"
#include "Rendering/RenderFrameDynamicData.h"
#include "Rendering/RenderWorldDelta.h"

#include <memory>
#include <span>
#include <string>
#include <vector>

class GpuSceneSlotAllocator;
class GPUMeshCache;
class RhiCommandSubmissionService;

struct RenderProxy final
{
	RenderObjectId Object;
	RenderObjectStaticData Static;
	RenderObjectDynamicData Dynamic;
	GpuMeshHandle GpuMesh;
	RenderObjectStaticData PendingStatic;
	GpuMeshHandle PendingGpuMesh;
	std::uint32_t GpuSceneSlot = 0;
	bool GpuMeshResident = false;
	bool GpuMeshFailed = false;
	bool HasPendingStatic = false;
};

enum class RenderWorldApplyStatus : std::uint8_t
{
	Applied,
	Duplicate,
	Stale,
	OutOfOrder,
	Rejected
};

class RenderWorld final
{
  public:
	RenderWorld(RhiCommandSubmissionService* submissionService, GPUMeshCache& gpuMeshCache);
	~RenderWorld() noexcept;

	RenderWorldApplyStatus ApplyFrame(
	    const RenderWorldDelta& delta,
	    const RenderFrameDynamicData& dynamic,
	    std::string& diagnostic);
	void PromoteResidentGpuMeshes() noexcept;
	const RenderProxy* Find(RenderObjectId object) const noexcept;
	std::span<const RenderProxy> GetProxies() const noexcept { return m_proxies; }
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
	RenderWorldApplyStatus ValidateDelta(
	    const RenderWorldDelta& delta,
	    std::string& diagnostic) const;
	bool ValidateDynamic(
	    const RenderFrameDynamicData& dynamic,
	    const RenderWorldDelta& delta,
	    std::string& diagnostic) const;
	RenderWorldApplyStatus ApplyValidatedDelta(
	    const RenderWorldDelta& delta,
	    std::string& diagnostic);
	void ApplyDynamic(const RenderFrameDynamicData& dynamic) noexcept;
	void ApplyDestroys(const RenderWorldDelta& delta);
	void ResolveGpuMeshes(
	    const RenderWorldDelta& delta,
	    std::vector<GpuMeshHandle>& createMeshes,
	    std::vector<GpuMeshHandle>& updateMeshes);
	void ApplyCreates(const RenderWorldDelta& delta, std::span<const GpuMeshHandle> meshes);
	void ApplyUpdates(const RenderWorldDelta& delta, std::span<const GpuMeshHandle> meshes);
	void PublishResources(const RenderWorldDelta& delta);
	void RetainReferencedGpuMeshes() noexcept;
	bool IsObjectAvailable(
	    RenderObjectId object,
	    const RenderWorldDelta& delta) const noexcept;
	static bool HasOrderedDeltaObjects(const RenderWorldDelta& delta) noexcept;
	static bool HasConflictingDeltaObjects(const RenderWorldDelta& delta) noexcept;
	static bool HasStrictlyOrderedDynamicObjects(
	    std::span<const RenderObjectDynamicData> objects) noexcept;
	static bool HasStrictlyOrderedSkinningObjects(
	    std::span<const RenderSkinningData> objects) noexcept;
	static bool HasStrictlyOrderedMorphObjects(
	    std::span<const RenderMorphData> objects) noexcept;
	RenderProxy* FindMutable(RenderObjectId object) noexcept;
	std::vector<RenderProxy> m_proxies;
	std::unique_ptr<GpuSceneSlotAllocator> m_gpuSceneSlots;
	GPUMeshCache* m_gpuMeshCache = nullptr;
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
