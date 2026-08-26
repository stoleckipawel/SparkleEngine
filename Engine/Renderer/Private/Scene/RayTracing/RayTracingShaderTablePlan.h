#pragma once

#include <cstdint>
#include <span>
#include <vector>

struct RenderMaterialTable;
struct RenderPrimitive;

enum class RayTracingSceneRayType : std::uint32_t
{
	Surface = 0u,
	ShadowVisibility = 1u,
	Count = 2u,
};

enum class RayTracingShaderTableHitGroup : std::uint8_t
{
	Opaque,
	AlphaTested,
};

enum class RayTracingShaderTableInvalidationReason : std::uint8_t
{
	None,
	RayTypeLayout,
	GeometryLayout,
	MaterialSemantics,
	SceneReset,
};

struct RayTracingShaderTableInstancePlan final
{
	std::uint32_t GpuSceneSlot = 0u;
	std::uint32_t InstanceContribution = 0u;
	std::uint32_t GeometryCount = 0u;
	std::uint64_t GeometryIdentity = 0u;
	RayTracingShaderTableHitGroup HitGroup = RayTracingShaderTableHitGroup::Opaque;

	bool operator==(const RayTracingShaderTableInstancePlan&) const noexcept = default;
};

struct RayTracingShaderTableRecordPlan final
{
	std::uint32_t RecordIndex = 0u;
	std::uint32_t GpuSceneSlot = 0u;
	std::uint32_t GeometryIndex = 0u;
	RayTracingSceneRayType RayType = RayTracingSceneRayType::Surface;
	RayTracingShaderTableHitGroup HitGroup = RayTracingShaderTableHitGroup::Opaque;

	bool operator==(const RayTracingShaderTableRecordPlan&) const noexcept = default;
};

struct RayTracingShaderTableMetrics final
{
	std::uint64_t Generation = 0u;
	std::uint64_t TableBytes = 0u;
	std::uint64_t LastBuildTimeMicroseconds = 0u;
	std::uint64_t LastUpdateTimeMicroseconds = 0u;
	std::uint32_t LogicalRecordCount = 0u;
	std::uint32_t MaterializedTableCount = 0u;
	std::uint32_t BuildCount = 0u;
	std::uint32_t UpdateCount = 0u;
	RayTracingShaderTableInvalidationReason InvalidationReason = RayTracingShaderTableInvalidationReason::None;
};

class RayTracingShaderTablePlan final
{
public:
	void Synchronize(
	    std::span<const RenderPrimitive> primitives,
	    const RenderMaterialTable& materials) noexcept;
	void Clear() noexcept;

	bool ResolveInstanceContribution(std::uint32_t gpuSceneSlot, std::uint32_t& contribution) const noexcept;
	bool ComputeRecordIndex(
	    std::uint32_t gpuSceneSlot,
	    std::uint32_t geometryIndex,
	    RayTracingSceneRayType rayType,
	    std::uint32_t& recordIndex) const noexcept;
	bool Validate() const noexcept;
	void BeginMaterializationSet() noexcept;
	void RecordMaterialization(std::uint64_t tableBytes, std::uint64_t elapsedMicroseconds) noexcept;
	static bool ComputeCheckedRecordIndex(
	    std::uint32_t rayContribution,
	    std::uint32_t geometryMultiplier,
	    std::uint32_t geometryIndex,
	    std::uint32_t instanceContribution,
	    std::uint32_t recordCount,
	    std::uint32_t& recordIndex) noexcept;

	std::span<const RayTracingShaderTableInstancePlan> GetInstances() const noexcept { return m_instances; }
	std::span<const RayTracingShaderTableRecordPlan> GetRecords() const noexcept { return m_records; }
	std::uint32_t GetGeometryMultiplier() const noexcept { return static_cast<std::uint32_t>(RayTracingSceneRayType::Count); }
	std::uint64_t GetGeneration() const noexcept { return m_generation; }
	const RayTracingShaderTableMetrics& GetMetrics() const noexcept { return m_metrics; }

private:
	const RayTracingShaderTableInstancePlan* FindInstance(std::uint32_t gpuSceneSlot) const noexcept;

	std::vector<RayTracingShaderTableInstancePlan> m_instances;
	std::vector<RayTracingShaderTableRecordPlan> m_records;
	std::uint64_t m_generation = 0u;
	RayTracingShaderTableMetrics m_metrics = {};
	bool m_hasMaterialized = false;
	bool m_recordingUpdate = false;
};
