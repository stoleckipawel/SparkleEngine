#pragma once

#include "Meshes/GpuMesh.h"
#include "Rendering/RenderSceneDelta.h"
#include "Rendering/RenderSceneDynamicData.h"

#include <memory>
#include <span>
#include <string>
#include <vector>

class GpuSceneSlotAllocator;
class GpuMeshCache;
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
	RenderWorld(RhiCommandSubmissionService* submissionService, GpuMeshCache& gpuMeshCache);
	~RenderWorld() noexcept;

	RenderWorldApplyStatus ApplyFrame(const RenderSceneDelta& delta, const RenderSceneDynamicData& dynamic, std::string& diagnostic);
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
	RenderWorldApplyStatus ValidateDelta(const RenderSceneDelta& delta, std::string& diagnostic) const;
	bool ValidateDynamic(const RenderSceneDynamicData& dynamic, const RenderSceneDelta& delta, std::string& diagnostic) const;
	RenderWorldApplyStatus ApplyValidatedDelta(const RenderSceneDelta& delta, std::string& diagnostic);
	void ApplyDynamic(const RenderSceneDynamicData& dynamic) noexcept;
	void ApplyDestroys(const RenderSceneDelta& delta);
	void ResolveGpuMeshes(
	    const RenderSceneDelta& delta,
	    std::vector<GpuMeshHandle>& createMeshes,
	    std::vector<GpuMeshHandle>& updateMeshes);
	void ApplyCreates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes);
	void ApplyUpdates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes);
	void PublishResources(const RenderSceneDelta& delta);
	void RetainReferencedGpuMeshes() noexcept;
	bool IsObjectAvailable(RenderObjectId object, const RenderSceneDelta& delta) const noexcept;
	static bool HasOrderedDeltaObjects(const RenderSceneDelta& delta) noexcept;
	static bool HasConflictingDeltaObjects(const RenderSceneDelta& delta) noexcept;
	static bool HasStrictlyOrderedDynamicObjects(std::span<const RenderObjectDynamicData> objects) noexcept;
	static bool HasStrictlyOrderedJointMatrixRanges(std::span<const RenderJointMatrixRange> ranges) noexcept;
	static bool HasStrictlyOrderedMorphWeightRanges(std::span<const RenderMorphWeightRange> ranges) noexcept;
	RenderProxy* FindMutable(RenderObjectId object) noexcept;
	std::vector<RenderProxy> m_proxies;
	std::unique_ptr<GpuSceneSlotAllocator> m_gpuSceneSlots;
	GpuMeshCache* m_gpuMeshCache = nullptr;
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
