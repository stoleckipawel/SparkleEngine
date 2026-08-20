#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GpuMesh.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/RayTracing/RhiRayTracingTransformPacking.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include <array>
#include <unordered_set>
#include <vector>

static const auto g_rayTracingPartitionedTlasBuildLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.PartitionedTlasBuild");

struct RayTracingPartitionedTlasStrategy::PartitionedBuildState final
{
	std::vector<RhiPartitionedTlasInstanceWriteDesc> InstanceWrites;
	std::unordered_set<void*> BuiltBlasResources;
	RhiPartitionedTlasOperationBufferLayout NativeOperationLayout = {};
};

RhiPartitionedTlasInstanceFlags RayTracingPartitionedTlasStrategy::ResolveInstanceFlags(
    const PreparedRenderScene& preparedScene,
    const MeshDraw& draw) noexcept
{
	RhiPartitionedTlasInstanceFlags flags = RhiPartitionedTlasInstanceFlags::None;
	if (draw.Material.Slot >= preparedScene.materials.size())
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasBuildLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS input references a material outside the render scene.");
	}
	const MaterialData& material = preparedScene.materials[draw.Material.Slot];
	if (material.doubleSided)
	{
		flags = flags | RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable;
	}
	if (material.alphaMode == 1u)
	{
		flags = flags | RhiPartitionedTlasInstanceFlags::ForceNoOpaque;
	}
	return flags;
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::BuildPartitionedTlas(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	RayTracingTopLevelAccelerationStructureBuildResult result{};
	result.ActiveProvider = ERhiRayTracingTopLevelProvider::PartitionedTlas;
	result.ActiveProviderReason = GetActiveProviderReason();
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;

	if (!CanUseActivePartitionedTlasProvider())
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasBuildLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS build has no usable device provider.");
	}

	const RayTracingPtlasPartitionPlan* partitionPlan = scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	EnsurePartitionedTlasResources(preparedScene, partitionPlan);

	PartitionedBuildState state;
	state.InstanceWrites.reserve(work.PartitionedTlasBlasInputIndices.size());
	CollectPartitionedInstances(commandContext, preparedScene, partitionPlan, blasCache, diagnostics, state);
	const std::uint32_t nativeWriteCount = static_cast<std::uint32_t>(state.InstanceWrites.size());
	result.Stats.InstanceCount = nativeWriteCount;
	PreparePartitionedOperationBuffer(state);

	RecordPartitionedBuild(commandContext, state, diagnostics);
	m_partitionedResources.InstanceCount = nativeWriteCount;
	m_partitionedResources.Built = true;
	return result;
}

void RayTracingPartitionedTlasStrategy::CollectPartitionedInstances(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan* partitionPlan,
    RayTracingBlasCache& blasCache,
    RayTracingPerformanceDiagnostics* diagnostics,
    PartitionedBuildState& state) noexcept
{
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	for (const std::uint32_t blasInputIndex : work.PartitionedTlasBlasInputIndices)
	{
		if (blasInputIndex >= work.BlasInputs.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPartitionedTlasBuildLogger,
			    __FILE__,
			    __LINE__,
			    "Partitioned TLAS work references a BLAS input outside the prepared work plan.");
		}
		const RenderRayTracingBlasInput& input = work.BlasInputs[blasInputIndex];
		if (input.PrimitiveIndex >= preparedScene.primitives.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPartitionedTlasBuildLogger,
			    __FILE__,
			    __LINE__,
			    "Partitioned TLAS work references a mesh instance outside the render scene.");
		}
		const MeshDraw& draw = preparedScene.primitives[input.PrimitiveIndex].Draw;
		if (!draw.Geometry.Mesh)
		{
			Diagnostics::Fatal(
			    g_rayTracingPartitionedTlasBuildLogger,
			    __FILE__,
			    __LINE__,
			    "Partitioned TLAS work references a mesh instance with no GPU mesh handle.");
		}

		const RayTracingBlasCache::BlasHandle blas =
		    blasCache.EnsureBlas(commandContext, preparedScene, draw, input.GpuSceneSlot, diagnostics);

		commandContext.TrackResource(blas.resource);
		if (blas.builtThisFrame)
		{
			state.BuiltBlasResources.insert(blas.resource.Value);
		}

		const RayTracingPtlasPartitionEntry* entry =
		    partitionPlan != nullptr ? partitionPlan->FindByPrimitive(input.PrimitiveIndex) : nullptr;
		if (entry == nullptr || !entry->Valid)
		{
			Diagnostics::Fatal(
			    g_rayTracingPartitionedTlasBuildLogger,
			    __FILE__,
			    __LINE__,
			    "Partitioned TLAS work has no valid partition-plan entry.");
		}

		state.InstanceWrites.push_back(
		    RhiPartitionedTlasInstanceWriteDesc{
		        .Transform = RhiRayTracingTransformPacking::PackCanonicalObjectToWorld(draw.Transform.WorldMatrix),
		        .ExplicitBoundingBox = {},
		        .InstanceID = input.GpuSceneSlot,
		        .InstanceMask = 0xFFu,
		        .InstanceContributionToHitGroupIndex = 0u,
		        .Flags = ResolveInstanceFlags(preparedScene, draw),
		        .InstanceIndex = input.GpuSceneSlot,
		        .PartitionIndex = entry->Assignment.PartitionId,
		        .AccelerationStructure = blas.gpuAddress});
	}
}

void RayTracingPartitionedTlasStrategy::PreparePartitionedOperationBuffer(PartitionedBuildState& state) noexcept
{
	const std::uint32_t nativeWriteCount = static_cast<std::uint32_t>(state.InstanceWrites.size());
	const RhiPartitionedTlasOperationHeader operation{
	    .Type = ERhiPartitionedTlasOperationType::WriteInstance,
	    .ArgumentCount = nativeWriteCount,
	    .ArgumentData = 0,
	    .ArgumentStrideInBytes = 0};
	const RhiPartitionedTlasOperationPackDesc operationPack{
	    .Operations = &operation,
	    .OperationCount = 1,
	    .InstanceWrites = state.InstanceWrites.data(),
	    .InstanceWriteCount = nativeWriteCount};

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_partitionedResources.NativeOperationData)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.NativeOperationData);
		m_partitionedResources.NativeOperationData = {};
	}
	{
		m_partitionedResources.NativeOperationData = rayTracingService.CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
		    operationPack,
		    L"RayTracingPartitionedTlasCpuPackedOperations");
	}
	if (!m_partitionedResources.NativeOperationData)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasBuildLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS operation-buffer allocation failed.");
	}

	state.NativeOperationLayout =
	    rayTracingService.GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(m_partitionedResources.Layout);
	m_partitionedResources.NativeOperationDataAddress =
	    resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.NativeOperationData);
	if (m_partitionedResources.NativeOperationDataAddress == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasBuildLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS operation buffer has no GPU address.");
	}
}

void RayTracingPartitionedTlasStrategy::RecordPartitionedBuild(
    RenderCommandContext& commandContext,
    const PartitionedBuildState& state,
    RayTracingPerformanceDiagnostics* diagnostics) const noexcept
{
	for (void* resourceValue : state.BuiltBlasResources)
	{
		commandContext.UnorderedAccessBarrier(RhiResourceHandle{resourceValue});
	}

	TrackBuildResources(commandContext);

	auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope("Partitioned TLAS Build") : ScopedGpuScope{};
	commandContext.BuildPartitionedTopLevelAccelerationStructure(
	    RhiPartitionedTlasBuildCommandDesc{
	        .Layout = m_partitionedResources.Layout,
	        .DestinationResource = m_renderHardwareInterface->GetResourceService().GetResourceHandle(m_partitionedResources.Storage),
	        .SourceAccelerationStructure = 0,
	        .DestinationAccelerationStructure = m_partitionedResources.StorageAddress,
	        .Scratch = m_partitionedResources.ScratchAddress,
	        .OperationHeaders =
	            m_partitionedResources.NativeOperationDataAddress + state.NativeOperationLayout.OperationHeadersOffsetInBytes,
	        .OperationCount = m_partitionedResources.NativeOperationDataAddress + state.NativeOperationLayout.OperationCountOffsetInBytes});
}

void RayTracingPartitionedTlasStrategy::TrackBuildResources(RenderCommandContext& commandContext) const noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return;
	}

	RhiResourceService& resources = m_renderHardwareInterface->GetResourceService();

	commandContext.TrackResource(resources.GetResourceHandle(m_partitionedResources.Storage));
	commandContext.TrackResource(resources.GetResourceHandle(m_partitionedResources.Scratch));
	commandContext.TrackResource(resources.GetResourceHandle(m_partitionedResources.NativeOperationData));
}
