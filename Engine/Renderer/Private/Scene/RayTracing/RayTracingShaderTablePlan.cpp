#include "PCH.h"

#include "Scene/RayTracing/RayTracingShaderTablePlan.h"

#include "Core/Public/Diagnostics/Verify.h"
#include "Rendering/RenderSceneDelta.h"
#include "RHI/Public/RayTracing/RhiAccelerationStructureDesc.h"
#include "Scene/RenderPrimitive.h"

#include <algorithm>
#include <limits>

static const auto g_rayTracingShaderTablePlanLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.ShaderTablePlan");

void RayTracingShaderTablePlan::Synchronize(std::span<const RenderPrimitive> primitives, const RenderMaterialTable& materials) noexcept
{
	std::vector<RayTracingShaderTableInstancePlan> instances;
	instances.reserve(primitives.size());
	for (const RenderPrimitive& primitive : primitives)
	{
		if (!primitive.GpuMeshResident || !primitive.GpuMesh || !primitive.Static.Material.IsValid()
		    || primitive.Static.Material.GetGeneration() != materials.Generation
		    || primitive.Static.Material.GetIndex() >= materials.Values.size())
		{
			continue;
		}

		const std::uint32_t materialIndex = primitive.Static.Material.GetIndex();
		const RayTracingShaderTableHitGroup hitGroup = materials.Values[materialIndex].alphaMode == AlphaMode::Mask
		    ? RayTracingShaderTableHitGroup::AlphaTested
		    : RayTracingShaderTableHitGroup::Opaque;
		instances.push_back(
		    RayTracingShaderTableInstancePlan{
		        .GpuSceneSlot = primitive.GpuSceneSlot,
		        .GeometryCount = 1u,
		        .GeometryIdentity = primitive.GpuMesh.Value,
		        .HitGroup = hitGroup});
	}
	std::ranges::sort(instances, {}, &RayTracingShaderTableInstancePlan::GpuSceneSlot);

	const std::uint32_t geometryMultiplier = static_cast<std::uint32_t>(RayTracingSceneRayType::Count);
	std::uint64_t nextInstanceContribution = 0u;
	for (RayTracingShaderTableInstancePlan& instance : instances)
	{
		if (nextInstanceContribution > kRhiRayTracingMaxInstanceContributionToHitGroupIndex)
		{
			Diagnostics::Fatal(
			    g_rayTracingShaderTablePlanLogger,
			    __FILE__,
			    __LINE__,
			    "Ray-tracing shader-table instance contribution exceeds the RHI descriptor contract.");
		}
		instance.InstanceContribution = static_cast<std::uint32_t>(nextInstanceContribution);
		nextInstanceContribution += static_cast<std::uint64_t>(geometryMultiplier) * instance.GeometryCount;
	}
	if (nextInstanceContribution > std::numeric_limits<std::uint32_t>::max())
	{
		Diagnostics::Fatal(
		    g_rayTracingShaderTablePlanLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing shader-table record count exceeds the Renderer/RHI index width.");
	}

	std::vector<RayTracingShaderTableRecordPlan> records;
	records.reserve(static_cast<std::size_t>(nextInstanceContribution));
	for (const RayTracingShaderTableInstancePlan& instance : instances)
	{
		for (std::uint32_t geometryIndex = 0u; geometryIndex < instance.GeometryCount; ++geometryIndex)
		{
			for (std::uint32_t rayContribution = 0u; rayContribution < geometryMultiplier; ++rayContribution)
			{
				std::uint32_t recordIndex = 0u;
				if (!ComputeCheckedRecordIndex(
				        rayContribution,
				        geometryMultiplier,
				        geometryIndex,
				        instance.InstanceContribution,
				        static_cast<std::uint32_t>(nextInstanceContribution),
				        recordIndex))
				{
					Diagnostics::Fatal(
					    g_rayTracingShaderTablePlanLogger,
					    __FILE__,
					    __LINE__,
					    "Ray-tracing shader-table record formula overflowed or escaped the planned table.");
				}
				records.push_back(
				    RayTracingShaderTableRecordPlan{
				        .RecordIndex = recordIndex,
				        .GpuSceneSlot = instance.GpuSceneSlot,
				        .GeometryIndex = geometryIndex,
				        .RayType = static_cast<RayTracingSceneRayType>(rayContribution),
				        .HitGroup = instance.HitGroup});
			}
		}
	}

	bool geometryChanged = instances.size() != m_instances.size();
	bool materialSemanticsChanged = false;
	for (std::size_t index = 0u; !geometryChanged && index < instances.size(); ++index)
	{
		const RayTracingShaderTableInstancePlan& incoming = instances[index];
		const RayTracingShaderTableInstancePlan& current = m_instances[index];
		geometryChanged = incoming.GpuSceneSlot != current.GpuSceneSlot || incoming.InstanceContribution != current.InstanceContribution
		    || incoming.GeometryCount != current.GeometryCount || incoming.GeometryIdentity != current.GeometryIdentity;
		materialSemanticsChanged = materialSemanticsChanged || incoming.HitGroup != current.HitGroup;
	}
	if (!geometryChanged && !materialSemanticsChanged && records == m_records)
	{
		return;
	}

	m_instances = std::move(instances);
	m_records = std::move(records);
	++m_generation;
	m_metrics.Generation = m_generation;
	m_metrics.LogicalRecordCount = static_cast<std::uint32_t>(m_records.size());
	m_metrics.MaterializedTableCount = 0u;
	m_metrics.TableBytes = 0u;
	m_metrics.LastBuildTimeMicroseconds = 0u;
	m_metrics.LastUpdateTimeMicroseconds = 0u;
	m_metrics.InvalidationReason = m_generation == 1u ? RayTracingShaderTableInvalidationReason::RayTypeLayout
	    : geometryChanged                             ? RayTracingShaderTableInvalidationReason::GeometryLayout
	                                                  : RayTracingShaderTableInvalidationReason::MaterialSemantics;
	if (!Validate())
	{
		Diagnostics::Fatal(
		    g_rayTracingShaderTablePlanLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing shader-table synchronization produced an invalid logical mapping.");
	}
}

void RayTracingShaderTablePlan::Clear() noexcept
{
	m_instances.clear();
	m_records.clear();
	++m_generation;
	m_metrics =
	    RayTracingShaderTableMetrics{.Generation = m_generation, .InvalidationReason = RayTracingShaderTableInvalidationReason::SceneReset};
	m_hasMaterialized = false;
	m_recordingUpdate = false;
}

bool RayTracingShaderTablePlan::ResolveInstanceContribution(std::uint32_t gpuSceneSlot, std::uint32_t& contribution) const noexcept
{
	const RayTracingShaderTableInstancePlan* instance = FindInstance(gpuSceneSlot);
	if (instance == nullptr)
	{
		return false;
	}
	contribution = instance->InstanceContribution;
	return true;
}

bool RayTracingShaderTablePlan::ComputeRecordIndex(
    std::uint32_t gpuSceneSlot,
    std::uint32_t geometryIndex,
    RayTracingSceneRayType rayType,
    std::uint32_t& recordIndex) const noexcept
{
	const RayTracingShaderTableInstancePlan* instance = FindInstance(gpuSceneSlot);
	const std::uint32_t rayContribution = static_cast<std::uint32_t>(rayType);
	if (instance == nullptr || geometryIndex >= instance->GeometryCount
	    || rayContribution >= static_cast<std::uint32_t>(RayTracingSceneRayType::Count))
	{
		return false;
	}
	return ComputeCheckedRecordIndex(
	    rayContribution,
	    GetGeometryMultiplier(),
	    geometryIndex,
	    instance->InstanceContribution,
	    static_cast<std::uint32_t>(m_records.size()),
	    recordIndex);
}

bool RayTracingShaderTablePlan::Validate() const noexcept
{
	if (m_records.size() > std::numeric_limits<std::uint32_t>::max())
	{
		return false;
	}
	for (std::size_t index = 0u; index < m_records.size(); ++index)
	{
		const RayTracingShaderTableRecordPlan& record = m_records[index];
		std::uint32_t computedIndex = 0u;
		if (record.RecordIndex != index || !ComputeRecordIndex(record.GpuSceneSlot, record.GeometryIndex, record.RayType, computedIndex)
		    || computedIndex != record.RecordIndex)
		{
			return false;
		}
	}
	return true;
}

void RayTracingShaderTablePlan::BeginMaterializationSet() noexcept
{
	m_metrics.TableBytes = 0u;
	m_metrics.MaterializedTableCount = 0u;
	m_metrics.LastBuildTimeMicroseconds = 0u;
	m_metrics.LastUpdateTimeMicroseconds = 0u;
	m_recordingUpdate = m_hasMaterialized;
}

void RayTracingShaderTablePlan::RecordMaterialization(std::uint64_t tableBytes, std::uint64_t elapsedMicroseconds) noexcept
{
	if (m_metrics.TableBytes > std::numeric_limits<std::uint64_t>::max() - tableBytes)
	{
		Diagnostics::Fatal(
		    g_rayTracingShaderTablePlanLogger,
		    __FILE__,
		    __LINE__,
		    "Ray-tracing shader-table byte metrics overflowed their bounded owner counter.");
	}
	m_metrics.TableBytes += tableBytes;
	++m_metrics.MaterializedTableCount;
	if (!m_recordingUpdate)
	{
		m_metrics.LastBuildTimeMicroseconds += elapsedMicroseconds;
		++m_metrics.BuildCount;
	}
	else
	{
		m_metrics.LastUpdateTimeMicroseconds += elapsedMicroseconds;
		++m_metrics.UpdateCount;
	}
	m_hasMaterialized = true;
}

const RayTracingShaderTableInstancePlan* RayTracingShaderTablePlan::FindInstance(std::uint32_t gpuSceneSlot) const noexcept
{
	const auto instance = std::ranges::lower_bound(m_instances, gpuSceneSlot, {}, &RayTracingShaderTableInstancePlan::GpuSceneSlot);
	return instance != m_instances.end() && instance->GpuSceneSlot == gpuSceneSlot ? &*instance : nullptr;
}

bool RayTracingShaderTablePlan::ComputeCheckedRecordIndex(
    std::uint32_t rayContribution,
    std::uint32_t geometryMultiplier,
    std::uint32_t geometryIndex,
    std::uint32_t instanceContribution,
    std::uint32_t recordCount,
    std::uint32_t& recordIndex) noexcept
{
	if (geometryMultiplier == 0u || rayContribution >= geometryMultiplier)
	{
		return false;
	}
	const std::uint64_t geometryTerm = static_cast<std::uint64_t>(geometryMultiplier) * geometryIndex;
	const std::uint64_t index = static_cast<std::uint64_t>(rayContribution) + geometryTerm + instanceContribution;
	if (index >= recordCount || index > std::numeric_limits<std::uint32_t>::max())
	{
		return false;
	}
	recordIndex = static_cast<std::uint32_t>(index);
	return true;
}
