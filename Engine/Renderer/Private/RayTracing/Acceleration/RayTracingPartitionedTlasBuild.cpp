#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <array>
#include <unordered_set>
#include <vector>

namespace RayTracingPartitionedTlasStrategyDetails
{
	std::array<float, 12> BuildInstanceTransform(const DirectX::XMFLOAT4X4& worldMatrix) noexcept
	{
		return {
		    worldMatrix._11,
		    worldMatrix._12,
		    worldMatrix._13,
		    worldMatrix._14,
		    worldMatrix._21,
		    worldMatrix._22,
		    worldMatrix._23,
		    worldMatrix._24,
		    worldMatrix._31,
		    worldMatrix._32,
		    worldMatrix._33,
		    worldMatrix._34};
	}

}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::BuildPartitionedTlas(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	RayTracingTopLevelAccelerationStructureBuildResult result{};
	result.ActiveProvider = ERhiRayTracingTopLevelProvider::PartitionedTlas;
	result.ActiveProviderReason = GetActiveProviderReason();
	result.Stats.Candidates.InstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());

	if (!CanUseActivePartitionedTlasProvider())
	{
		ReleasePartitionedTlasResources();
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = GetActiveProviderReason();
		return result;
	}

	const RayTracingPtlasPartitionPlan* partitionPlan =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	if (!EnsurePartitionedTlasResources(sceneData, partitionPlan))
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-resource-setup-failed-after-frame-prepare";
		return result;
	}
	auto resolveInstanceFlags = [&](const MeshDraw& draw) noexcept {
		RhiPartitionedTlasInstanceFlags flags = RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable;
		if (draw.Material.Slot < sceneData.materials.size() && sceneData.materials[draw.Material.Slot].alphaMode == 1u)
		{
			flags = flags | RhiPartitionedTlasInstanceFlags::ForceNoOpaque;
		}
		return flags;
	};

	std::vector<RhiPartitionedTlasInstanceWriteDesc> instanceWrites;
	std::unordered_set<void*> builtBlasResources;
	instanceWrites.reserve(sceneData.meshInstances.size());
	for (std::uint32_t renderInstanceIndex = 0;
	     renderInstanceIndex < static_cast<std::uint32_t>(sceneData.meshInstances.size());
	     ++renderInstanceIndex)
	{
		const MeshDraw& draw = sceneData.meshInstances[renderInstanceIndex];
		if (draw.Geometry.GpuMesh == nullptr || !draw.Geometry.GpuMesh->IsValid())
		{
			++result.Stats.Candidates.MissingGpuMeshCount;
			continue;
		}

		const RayTracingBlasCache::BlasHandle blas =
		    blasCache.EnsureBlas(cmd, sceneData, draw, renderInstanceIndex, diagnostics);
		if (!blas.IsValid())
		{
			++result.Stats.Candidates.RejectedBlasCount;
			continue;
		}
		if (blas.builtThisFrame)
		{
			builtBlasResources.insert(blas.resource.Value);
		}

		const RayTracingPtlasPartitionEntry* entry =
		    partitionPlan != nullptr ? partitionPlan->FindByRenderInstance(renderInstanceIndex) : nullptr;
		if (entry != nullptr && !entry->Valid)
		{
			continue;
		}

		instanceWrites.push_back(
		    RhiPartitionedTlasInstanceWriteDesc{
		        .Transform = RayTracingPartitionedTlasStrategyDetails::BuildInstanceTransform(draw.Transform.WorldMatrix),
		        .ExplicitBoundingBox = {},
		        .InstanceID = renderInstanceIndex,
		        .InstanceMask = 0xFFu,
		        .InstanceContributionToHitGroupIndex = 0u,
		        .Flags = resolveInstanceFlags(draw),
		        .InstanceIndex = draw.Source.SourceInstanceIndex,
		        .PartitionIndex = entry != nullptr ? entry->Assignment.PartitionId : 0u,
		        .AccelerationStructure = blas.gpuAddress});
	}

	const std::uint32_t nativeWriteCount = static_cast<std::uint32_t>(instanceWrites.size());
	result.Stats.Build.InstanceCount = nativeWriteCount;
	if (instanceWrites.empty())
	{
		InvalidatePartitionedTlasSceneState();
		return result;
	}

	const RhiPartitionedTlasOperationHeader operation{
	    .Type = ERhiPartitionedTlasOperationType::WriteInstance,
	    .ArgumentCount = nativeWriteCount,
	    .ArgumentData = 0,
	    .ArgumentStrideInBytes = 0};
	const RhiPartitionedTlasOperationPackDesc operationPack{
	    .Operations = &operation,
	    .OperationCount = 1,
	    .InstanceWrites = instanceWrites.data(),
	    .InstanceWriteCount = nativeWriteCount};

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_partitionedResources.NativeOperationData)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.NativeOperationData);
		m_partitionedResources.NativeOperationData = {};
	}
	{
		m_partitionedResources.NativeOperationData =
		    rayTracingService.CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
		        operationPack,
		        L"RayTracingPartitionedTlasCpuPackedOperations");
	}
	if (!m_partitionedResources.NativeOperationData)
	{
		InvalidatePartitionedTlasSceneState();
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-operation-pack-failed";
		return result;
	}

	const RhiPartitionedTlasOperationBufferLayout nativeOperationLayout =
	    rayTracingService.GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(m_partitionedResources.Layout);
	m_partitionedResources.NativeOperationDataAddress =
	    resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.NativeOperationData);
	if (m_partitionedResources.NativeOperationDataAddress == 0)
	{
		InvalidatePartitionedTlasSceneState();
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-operation-buffer-address-missing";
		return result;
	}

	for (void* resourceValue : builtBlasResources)
	{
		cmd.UnorderedAccessBarrier(NativeResourceHandle{resourceValue});
	}

	{
		auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuScope("Partitioned TLAS Build") : ScopedGpuScope{};
		cmd.BuildPartitionedTopLevelAccelerationStructure(
		    RhiPartitionedTlasBuildCommandDesc{
		        .Layout = m_partitionedResources.Layout,
		        .SourceAccelerationStructure = 0,
		        .DestinationAccelerationStructure = m_partitionedResources.StorageAddress,
		        .Scratch = m_partitionedResources.ScratchAddress,
		        .OperationHeaders =
		            m_partitionedResources.NativeOperationDataAddress +
		            nativeOperationLayout.OperationHeadersOffsetInBytes,
		        .OperationCount =
		            m_partitionedResources.NativeOperationDataAddress +
		            nativeOperationLayout.OperationCountOffsetInBytes});
	}

	result.Stats.Build.Built = true;
	m_partitionedResources.InstanceCount = nativeWriteCount;
	m_partitionedResources.Built = true;
	return result;
}
