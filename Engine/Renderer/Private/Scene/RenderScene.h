#pragma once

#include "Scene/RenderPrimitive.h"
#include "Rendering/RenderSceneDelta.h"
#include "Rendering/RenderSceneDynamicData.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"

#include <DirectXMath.h>

#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <vector>

class GpuSceneSlotAllocator;
class GpuMeshCache;
class MaterialCache;
class PassExecutionDiagnostics;
class RenderCommandContext;
class RenderGpuScene;
class RenderHardwareInterface;
class RenderRayTracingScene;
class RhiCommandSubmissionService;
class TextureCache;
struct RayTracingCapabilityReport;
struct PreparedRenderPrimitive;
struct RenderDeformationWork;
struct PreparedRenderScene;
struct RayTracingPtlasPartitionPlan;
struct RenderRayTracingFrameBindings;
struct RenderSceneGpuBindings;
struct RenderView;

class RenderScene final
{
public:
	RenderScene(
	    RhiCommandSubmissionService* submissionService,
	    GpuMeshCache& gpuMeshCache,
	    TextureCache& textureCache,
	    RenderHardwareInterface& renderHardwareInterface,
	    const RayTracingCapabilityReport& rayTracingCapabilities);
	~RenderScene() noexcept;

	bool Apply(const RenderSceneDelta& delta, RenderSceneDynamicData dynamic);
	void PromoteResidentGpuMeshes() noexcept;
	void BuildMaterials(PreparedRenderScene& preparedScene);
	const RenderSceneGpuBindings& UpdateGpuScene(
	    const PreparedRenderScene& preparedScene,
	    const RenderView& view,
	    std::uint32_t frameIndex);
	RenderRayTracingFrameBindings PrepareRayTracingFrame(
	    const PreparedRenderScene& preparedScene,
	    const RayTracingPtlasPartitionPlan& viewPlan) noexcept;
	bool IsRayTracingAvailable() const noexcept;
	RenderRayTracingScene& GetRayTracingScene() noexcept { return *m_renderRayTracingScene; }
	const RenderRayTracingScene& GetRayTracingScene() const noexcept { return *m_renderRayTracingScene; }
	void CommitContinuity(std::span<const PreparedRenderPrimitive> primitives, const RenderDeformationWork& deformation);
	void ResetContinuity() noexcept;
	DirectX::XMFLOAT4X4 ResolvePreviousWorldMatrix(const RenderPrimitive& primitive) const noexcept;
	std::span<const DirectX::XMFLOAT4X4> FindPreviousJointMatrices(RenderObjectId primitiveId) const noexcept;
	std::span<const float> FindPreviousMorphWeights(RenderObjectId primitiveId) const noexcept;
	const RenderPrimitive* Find(RenderObjectId primitiveId) const noexcept;
	std::span<const RenderPrimitive> GetPrimitives() const noexcept { return m_primitives; }
	const RenderMaterialTable& GetMaterials() const noexcept { return m_materials; }
	const RenderTextureTable& GetTextures() const noexcept { return m_textures; }
	const std::optional<SceneSkyDesc>& GetSky() const noexcept { return m_sky; }
	const std::vector<RenderMeshInstanceGroupData>& GetInstanceGroups() const noexcept { return m_instanceGroups; }
	std::span<const RenderLightData> GetLights() const noexcept { return m_lights; }
	std::span<const RenderJointMatrixRange> GetJointMatrixRanges() const noexcept { return m_jointMatrixRanges; }
	std::span<const DirectX::XMFLOAT4X4> GetJointMatrices() const noexcept { return m_jointMatrices; }
	std::span<const RenderMorphWeightRange> GetMorphWeightRanges() const noexcept { return m_morphWeightRanges; }
	std::span<const float> GetMorphWeights() const noexcept { return m_morphWeights; }
	std::uint64_t GetSceneGeneration() const noexcept { return m_sceneGeneration; }
	std::uint64_t GetSequenceNumber() const noexcept { return m_sequenceNumber; }
	std::uint64_t GetStructuralRevision() const noexcept { return m_structuralRevision; }
	std::uint64_t GetMaterialRevision() const noexcept { return m_materialRevision; }

private:
	bool ValidateDelta(const RenderSceneDelta& delta) const;
	bool ValidateDynamic(const RenderSceneDynamicData& dynamic, const RenderSceneDelta& delta) const;
	void ApplyValidatedDelta(const RenderSceneDelta& delta);
	void ApplyDynamic(RenderSceneDynamicData&& dynamic) noexcept;
	void ApplyDestroys(const RenderSceneDelta& delta);
	void ResolveGpuMeshes(
	    const RenderSceneDelta& delta,
	    std::vector<GpuMeshHandle>& createMeshes,
	    std::vector<GpuMeshHandle>& updateMeshes);
	void ApplyCreates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes);
	void ApplyUpdates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes);
	void PublishResources(const RenderSceneDelta& delta);
	void RetainReferencedGpuMeshes() noexcept;
	void CommitPreviousWorldTransforms(std::span<const PreparedRenderPrimitive> primitives);
	void CommitJointMatrixContinuity(const RenderDeformationWork& deformation);
	void CommitMorphWeightContinuity(const RenderDeformationWork& deformation);
	bool IsObjectAvailable(RenderObjectId primitiveId, const RenderSceneDelta& delta) const noexcept;
	static bool HasOrderedDeltaObjects(const RenderSceneDelta& delta) noexcept;
	static bool HasConflictingDeltaObjects(const RenderSceneDelta& delta) noexcept;
	static bool HasStrictlyOrderedDynamicObjects(std::span<const RenderObjectDynamicData> primitives) noexcept;
	static bool HasStrictlyOrderedJointMatrixRanges(std::span<const RenderJointMatrixRange> ranges) noexcept;
	static bool HasStrictlyOrderedMorphWeightRanges(std::span<const RenderMorphWeightRange> ranges) noexcept;
	RenderPrimitive* FindMutable(RenderObjectId primitiveId) noexcept;
	struct PreviousWorldTransform final
	{
		RenderObjectId Object;
		DirectX::XMFLOAT4X4 WorldMatrix = {};
	};

	std::vector<RenderPrimitive> m_primitives;
	std::unique_ptr<GpuSceneSlotAllocator> m_gpuSceneSlots;
	std::unique_ptr<RenderGpuScene> m_renderGpuScene;
	std::unique_ptr<RenderRayTracingScene> m_renderRayTracingScene;
	GpuMeshCache* m_gpuMeshCache = nullptr;
	std::unique_ptr<MaterialCache> m_materialCache;
	RenderMaterialTable m_materials;
	RenderTextureTable m_textures;
	std::optional<SceneSkyDesc> m_sky;
	std::vector<RenderMeshInstanceGroupData> m_instanceGroups;
	std::vector<RenderLightData> m_lights;
	std::vector<RenderJointMatrixRange> m_jointMatrixRanges;
	std::vector<DirectX::XMFLOAT4X4> m_jointMatrices;
	std::vector<RenderMorphWeightRange> m_morphWeightRanges;
	std::vector<float> m_morphWeights;
	std::vector<PreviousWorldTransform> m_previousWorldTransforms;
	std::map<RenderObjectId, std::vector<DirectX::XMFLOAT4X4>> m_jointMatrixHistory;
	std::map<RenderObjectId, std::vector<float>> m_morphWeightHistory;
	std::uint64_t m_sceneGeneration = 0;
	std::uint64_t m_sequenceNumber = 0;
	std::uint64_t m_structuralRevision = 0;
	std::uint64_t m_materialRevision = 0;
};
