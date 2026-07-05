#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/Acceleration/RayTracingBlasCache.h"
#include "RayTracing/Acceleration/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
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

	bool HasStructuralPartitionValidationFailure(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
	{
		return partitionPlan != nullptr &&
		       (partitionPlan->Validation.HasDuplicateStableIndices || partitionPlan->Validation.HasPartitionOverflow);
	}

	std::uint64_t ResolveStableInstanceFingerprint(const RenderSceneData& sceneData) noexcept
	{
		constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
		constexpr std::uint64_t prime = 1099511628211ull;

		std::uint64_t fingerprint = offsetBasis;
		for (const MeshDraw& draw : sceneData.meshInstances)
		{
			fingerprint ^= static_cast<std::uint64_t>(draw.Source.SourceInstanceIndex) + 0x9E3779B97F4A7C15ull;
			fingerprint *= prime;
		}
		fingerprint ^= static_cast<std::uint64_t>(sceneData.meshInstances.size());
		fingerprint *= prime;
		return fingerprint;
	}

	const char* ResolveActiveCpuPackReason(const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		switch (capabilityReport.Provider)
		{
			case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
				return "d3d12-nvapi-partitioned-tlas-active-cpu-pack";
			case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
				return "vulkan-nv-partitioned-tlas-active-cpu-pack";
			case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
				return "d3d12-public-dxr-partitioned-tlas-active-cpu-pack";
			case ERhiPartitionedTlasProvider::None:
			default:
				return "partitioned-tlas-active-cpu-pack";
		}
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

	const RayTracingPtlasPartitionPlan* partitionPlan =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	if (!EnsurePartitionedTlasResources(sceneData, partitionPlan))
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-resource-setup-failed-after-frame-prepare";
		return result;
	}
	const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentLogicalUpdateStream() : nullptr;
	const bool partitionPlanRequiresFullBuild =
	    RayTracingPartitionedTlasStrategyDetails::HasStructuralPartitionValidationFailure(partitionPlan);
	const std::uint64_t stableInstanceFingerprint =
	    RayTracingPartitionedTlasStrategyDetails::ResolveStableInstanceFingerprint(sceneData);
	bool useFullBuild =
	    !m_partitionedResources.Built || !m_partitionedResources.IncrementalUpdatesAllowed ||
	    partitionPlanRequiresFullBuild || m_partitionedResources.StableInstanceFingerprint != stableInstanceFingerprint;
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
	instanceWrites.reserve(
	    useFullBuild ? sceneData.meshInstances.size() : (logicalUpdates != nullptr ? logicalUpdates->LogicalUpdateCount : 0u));
	auto appendFullBuildWrites = [&]() {
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

			const RayTracingBlasCache::BlasHandle blas = blasCache.EnsureBlas(cmd, *draw.Geometry.GpuMesh, diagnostics);
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
			if (entry != nullptr && !entry->Validation.Valid)
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
	};
	auto appendIncrementalWrites = [&]() -> bool {
		if (logicalUpdates == nullptr)
		{
			return true;
		}

		for (const RayTracingPtlasLogicalUpdateRecord& record : logicalUpdates->Records)
		{
			if (record.RenderInstanceIndex >= sceneData.meshInstances.size())
			{
				return false;
			}

			const RayTracingPtlasPartitionEntry* entry =
			    partitionPlan != nullptr ? partitionPlan->FindByRenderInstance(record.RenderInstanceIndex) : nullptr;
			if (entry != nullptr && !entry->Validation.Valid)
			{
				return false;
			}

			const MeshDraw& draw = sceneData.meshInstances[record.RenderInstanceIndex];
			if (draw.Geometry.GpuMesh == nullptr || !draw.Geometry.GpuMesh->IsValid())
			{
				return false;
			}

			const RayTracingBlasCache::BlasHandle blas = blasCache.EnsureBlas(cmd, *draw.Geometry.GpuMesh, diagnostics);
			if (!blas.IsValid())
			{
				return false;
			}
			if (blas.builtThisFrame)
			{
				builtBlasResources.insert(blas.resource.Value);
			}

			instanceWrites.push_back(
			    RhiPartitionedTlasInstanceWriteDesc{
			        .Transform = record.Transform,
			        .ExplicitBoundingBox = {},
			        .InstanceID = record.InstanceID,
			        .InstanceMask = record.InstanceMask,
			        .InstanceContributionToHitGroupIndex = record.InstanceContributionToHitGroupIndex,
			        .Flags = record.InstanceFlags,
			        .InstanceIndex = record.InstanceIndex,
			        .PartitionIndex = record.PartitionIndex,
			        .AccelerationStructure = blas.gpuAddress});
		}

		return true;
	};
	{
		if (useFullBuild)
		{
			appendFullBuildWrites();
		}
		else if (!appendIncrementalWrites())
		{
			useFullBuild = true;
			instanceWrites.clear();
			builtBlasResources.clear();
			result.Stats.Candidates.MissingGpuMeshCount = 0;
			result.Stats.Candidates.RejectedBlasCount = 0;
			appendFullBuildWrites();
		}
	}

	const std::uint32_t nativeWriteCount = static_cast<std::uint32_t>(instanceWrites.size());
	result.Stats.Build.InstanceCount =
	    useFullBuild ? nativeWriteCount : (std::max)(m_partitionedResources.InstanceCount, nativeWriteCount);
	if (instanceWrites.empty())
	{
		m_partitionedResources.NativeOperationCount = 0;
		if (useFullBuild)
		{
			InvalidatePartitionedTlasSceneState();
		}
		else
		{
			result.Stats.Build.Built = m_partitionedResources.Built;
			m_activeProviderReason =
			    RayTracingPartitionedTlasStrategyDetails::ResolveActiveCpuPackReason(m_capabilityReport.PartitionedTlas);
			result.ActiveProviderReason = m_activeProviderReason;
		}
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

	m_partitionedResources.NativeOperationLayout =
	    rayTracingService.GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(m_partitionedResources.Layout);
	m_partitionedResources.NativeOperationDataAddress =
	    resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.NativeOperationData);
	m_partitionedResources.NativeOperationCount = nativeWriteCount;
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
		        .SourceAccelerationStructure = useFullBuild ? 0 : m_partitionedResources.StorageAddress,
		        .DestinationAccelerationStructure = m_partitionedResources.StorageAddress,
		        .Scratch = m_partitionedResources.ScratchAddress,
		        .OperationHeaders =
		            m_partitionedResources.NativeOperationDataAddress +
		            m_partitionedResources.NativeOperationLayout.OperationHeadersOffsetInBytes,
		        .OperationCount =
		            m_partitionedResources.NativeOperationDataAddress +
		            m_partitionedResources.NativeOperationLayout.OperationCountOffsetInBytes});
	}

	result.Stats.Build.Built = true;
	if (useFullBuild)
	{
		m_partitionedResources.InstanceCount = nativeWriteCount;
		m_partitionedResources.StableInstanceFingerprint = stableInstanceFingerprint;
		m_partitionedResources.IncrementalUpdatesAllowed =
		    !partitionPlanRequiresFullBuild && result.Stats.Candidates.MissingGpuMeshCount == 0 &&
		    result.Stats.Candidates.RejectedBlasCount == 0;
	}
	m_partitionedResources.Built = true;
	m_activeProviderReason =
	    RayTracingPartitionedTlasStrategyDetails::ResolveActiveCpuPackReason(m_capabilityReport.PartitionedTlas);
	result.ActiveProviderReason = m_activeProviderReason;
	return result;
}
