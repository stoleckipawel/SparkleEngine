#pragma once

#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"

#include <DirectXMath.h>

#include <cstdint>
#include <limits>
#include <vector>

struct MeshDraw;
struct RenderSceneData;

inline constexpr std::uint32_t kRayTracingPtlasInvalidEntryIndex = (std::numeric_limits<std::uint32_t>::max)();

struct RayTracingPtlasPartitionPlannerConfig final
{
	std::uint32_t PartitionsPerAxis = 8;
	RayTracingPtlasPartitionUpdateMode PartitionUpdateMode = RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool MarkAllDynamicInPartition = false;
	DirectX::XMFLOAT3 CameraPosition = {};
	float ModeChangeDistance = 100.0f;
	float TransformDirtyEpsilon = 0.0001f;
};

struct RayTracingPtlasPartitionEntryIdentity final
{
	std::uint32_t StableInstanceIndex = 0;
	std::uint32_t RenderInstanceIndex = 0;
	std::uint32_t GpuSceneSlot = 0;
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

struct RayTracingPtlasPartitionEntry final
{
	RayTracingPtlasPartitionEntryIdentity Identity;
	RayTracingPtlasPartitionAssignment Assignment;
	RayTracingPtlasPartitionUpdateState Update;
	bool Valid = true;
};

struct RayTracingPtlasPartitionPlanIndices final
{
	std::vector<RayTracingPtlasPartitionEntry> Entries;
	std::vector<std::uint32_t> RenderInstanceToEntry;
};

struct RayTracingPtlasPartitionPlanCounts final
{
	std::uint32_t PartitionsPerAxis = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t GridPartitionCount = 0;
	std::uint32_t GlobalPartitionIndex = kRayTracingPtlasInvalidEntryIndex;
	std::uint32_t MaxInstancesPerPartition = 0;
	std::uint32_t MaxInstancesInGlobalPartition = 0;
};

struct RayTracingPtlasPartitionPlanValidation final
{
	bool HasDuplicateStableIndices = false;
	bool HasPartitionOverflow = false;
};

struct RayTracingPtlasPartitionPlan final
{
	RayTracingPtlasPartitionPlanIndices Indices;
	RayTracingPtlasPartitionPlanCounts Counts;
	RayTracingPtlasPartitionPlanValidation Validation;

	const RayTracingPtlasPartitionEntry* FindByRenderInstance(std::uint32_t renderInstanceIndex) const noexcept;
};

class RayTracingPtlasPartitionPlanner final
{
  public:
	RayTracingPtlasPartitionPlanner() noexcept;

	RayTracingPtlasPartitionPlan Build(const RenderSceneData& sceneData, const RayTracingPtlasPartitionPlannerConfig& config) noexcept;
	void Clear() noexcept;

  private:
	struct SceneBounds;
	struct InstanceBounds;
	struct ObservedInstance;

	struct PreviousInstanceState final
	{
		DirectX::XMFLOAT4X4 WorldMatrix = {};
		std::uint32_t LocalPartitionId = 0;
		std::uint32_t PartitionId = 0;
		bool Valid = false;
	};

	struct PartitionRuntimeState final
	{
		bool TouchedThisFrame = false;
		bool FarFromCamera = false;
	};

	static RayTracingPtlasPartitionPlannerConfig SanitizeConfig(RayTracingPtlasPartitionPlannerConfig config) noexcept;
	static void ExpandSceneBounds(SceneBounds& bounds, const DirectX::XMFLOAT3& point) noexcept;
	static DirectX::XMFLOAT3 TransformPoint(
	    const DirectX::XMFLOAT3& point,
	    const DirectX::XMFLOAT4X4& worldMatrix) noexcept;
	static InstanceBounds ComputeInstanceWorldBounds(const MeshDraw& draw) noexcept;
	static DirectX::XMFLOAT3 ComputeInstancePartitionPosition(const MeshDraw& draw) noexcept;
	static SceneBounds ComputeSceneBounds(const RenderSceneData& sceneData) noexcept;
	static std::uint32_t QuantizeAxis(
	    float value,
	    float minValue,
	    float maxValue,
	    std::uint32_t partitionsPerAxis) noexcept;
	static std::uint32_t ComputeGridPartitionId(
	    const DirectX::XMFLOAT3& position,
	    const SceneBounds& bounds,
	    std::uint32_t partitionsPerAxis) noexcept;
	static DirectX::XMFLOAT3 ComputeGridPartitionCenter(
	    std::uint32_t partitionId,
	    const SceneBounds& bounds,
	    std::uint32_t partitionsPerAxis) noexcept;
	static std::uint64_t ComputeGridPartitionCount(std::uint32_t partitionsPerAxis) noexcept;
	static bool RequiresGlobalPartition(RayTracingPtlasPartitionUpdateMode updateMode) noexcept;
	static float DistanceSquared(
	    const DirectX::XMFLOAT3& lhs,
	    const DirectX::XMFLOAT3& rhs) noexcept;
	static bool IsTransformDirty(
	    const DirectX::XMFLOAT4X4& current,
	    const DirectX::XMFLOAT4X4& previous,
	    float epsilon) noexcept;
	static bool IsGlobalPartitionEligible(const MeshDraw& draw) noexcept;

	std::vector<PreviousInstanceState> m_previousInstances;
	std::vector<PartitionRuntimeState> m_partitionStates;
};
