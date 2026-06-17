#pragma once

#include "Renderer/Public/Debug/RendererCVars.h"
#include "Renderer/Public/SceneData/RenderMeshClassification.h"

#include <DirectXMath.h>

#include <cstdint>
#include <vector>

struct MeshDraw;
struct RenderSceneData;

inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugPartitionMask = 0x000FFFFFu;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugActivityShift = 20u;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugActivityMask = 0x0FF00000u;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugDirtyTransform = 1u << 28u;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugMovedPartition = 1u << 29u;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugGlobalPartition = 1u << 30u;
inline constexpr std::uint32_t kRayTracingPtlasPartitionDebugInvalid = 1u << 31u;

struct RayTracingPtlasPartitionPlannerConfig final
{
	std::uint32_t PartitionsPerAxis = 8;
	RayTracingPtlasPartitionTopology PartitionTopology = RayTracingPtlasPartitionTopology::XYZ3D;
	RayTracingPtlasPartitionUpdateMode PartitionUpdateMode = RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool EnableGlobalPartition = true;
	bool MarkAllDynamicInPartition = false;
	DirectX::XMFLOAT3 CameraPosition = {};
	float ModeChangeDistance = 100.0f;
	float TransformDirtyEpsilon = 0.0001f;
};

struct RayTracingPtlasPartitionEntryIdentity final
{
	std::uint32_t StableInstanceIndex = 0;
	std::uint32_t RenderInstanceIndex = 0;
	std::uint32_t SourceInstanceIndex = 0;
};

struct RayTracingPtlasPartitionAssignment final
{
	std::uint32_t PartitionId = 0;
	std::uint32_t PreviousPartitionId = 0;
};

struct RayTracingPtlasPartitionUpdateState final
{
	bool DirtyTransform = true;
	bool MovedPartition = false;
	bool GlobalPartitionEligible = false;
	bool UsesGlobalPartition = false;
};

struct RayTracingPtlasPartitionValidation final
{
	bool DuplicateStableIndex = false;
	bool Valid = true;
};

struct RayTracingPtlasPartitionDebugVisualization final
{
	std::uint32_t PackedData = 0;
	std::uint32_t ActivityLevel = 0;
};

struct RayTracingPtlasPartitionEntry final
{
	RayTracingPtlasPartitionEntryIdentity Identity;
	RayTracingPtlasPartitionAssignment Assignment;
	RayTracingPtlasPartitionUpdateState Update;
	RayTracingPtlasPartitionValidation Validation;
	RayTracingPtlasPartitionDebugVisualization DebugVisualization;
};

struct RayTracingPtlasPartitionPlanIndices final
{
	std::vector<RayTracingPtlasPartitionEntry> Entries;
	std::vector<std::uint32_t> RenderInstanceToEntry;
};

struct RayTracingPtlasPartitionPlanCounts final
{
	std::uint32_t CandidateInstanceCount = 0;
	std::uint32_t StaticInstanceCount = 0;
	std::uint32_t DynamicInstanceCount = 0;
	std::uint32_t PartitionsPerAxis = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t GridPartitionCount = 0;
	std::uint32_t DirtyTransformCount = 0;
	std::uint32_t MovedPartitionCount = 0;
	std::uint32_t GlobalPartitionEligibleCount = 0;
	std::uint32_t GlobalPartitionInstanceCount = 0;
	std::uint32_t ActivePartitionCount = 0;
	std::uint32_t MaxPartitionActivityCount = 0;
	std::uint32_t DuplicateStableIndexCount = 0;
};

struct RayTracingPtlasPartitionPlanValidation final
{
	bool HasDuplicateStableIndices = false;
	bool HasPartitionOverflow = false;
	bool HasInvalidPartition = false;
};

struct RayTracingPtlasPartitionPlan final
{
	RayTracingPtlasPartitionPlanIndices Indices;
	RayTracingPtlasPartitionPlanCounts Counts;
	RayTracingPtlasPartitionPlanValidation Validation;

	const RayTracingPtlasPartitionEntry* FindByRenderInstance(std::uint32_t renderInstanceIndex) const noexcept;
	std::uint32_t GetPackedDebugVisualizationDataForRenderInstance(std::uint32_t renderInstanceIndex) const noexcept;
};

class RayTracingPtlasPartitionPlanner final
{
  public:
	RayTracingPtlasPartitionPlanner() noexcept = default;

	RayTracingPtlasPartitionPlan Build(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlannerConfig& config) noexcept;
	void Clear() noexcept;

  private:
	struct PreviousInstanceState final
	{
		DirectX::XMFLOAT4X4 WorldMatrix = {};
		std::uint32_t LocalPartitionId = 0;
		std::uint32_t PartitionId = 0;
		bool Valid = false;
	};

	struct PartitionRuntimeState final
	{
		std::uint32_t LastModifiedFrame = 0;
		std::uint32_t ActivityCountThisFrame = 0;
		bool TouchedThisFrame = false;
		bool FarFromCamera = false;
	};

	static RayTracingPtlasPartitionPlannerConfig SanitizeConfig(RayTracingPtlasPartitionPlannerConfig config) noexcept;
	static bool IsTransformDirty(
	    const DirectX::XMFLOAT4X4& current,
	    const DirectX::XMFLOAT4X4& previous,
	    float epsilon) noexcept;
	static bool IsGlobalPartitionEligible(const MeshDraw& draw) noexcept;
	static std::uint32_t PackDebugVisualizationData(const RayTracingPtlasPartitionEntry& entry) noexcept;

	std::vector<PreviousInstanceState> m_previousInstances;
	std::vector<PartitionRuntimeState> m_partitionStates;
	std::uint32_t m_frameIndex = 0;
};
