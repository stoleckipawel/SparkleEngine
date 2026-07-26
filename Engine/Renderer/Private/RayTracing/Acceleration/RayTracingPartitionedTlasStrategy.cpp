#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>

bool RayTracingPartitionedTlasStrategy::IsUsablePartitionPlan(
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	return partitionPlan != nullptr &&
	       !partitionPlan->Validation.HasDuplicateStableIndices &&
	       !partitionPlan->Validation.HasPartitionOverflow &&
	       partitionPlan->Counts.PartitionCount != 0;
}

std::uint32_t
RayTracingPartitionedTlasStrategy::ResolveInstanceCapacity(
    const RenderSceneData& sceneData) noexcept
{
	std::uint32_t instanceCapacity = 0;
	const RenderRayTracingWorkPlan& work =
	    sceneData.rayTracingWork;
	for (const std::uint32_t blasInputIndex :
	     work.PartitionedTlasBlasInputIndices)
	{
		if (blasInputIndex >= work.BlasInputs.size())
		{
			continue;
		}
		const RenderRayTracingBlasInput& input =
		    work.BlasInputs[blasInputIndex];
		instanceCapacity =
		    (std::max)(
		        instanceCapacity,
		        input.GpuSceneSlot + 1u);
	}
	return instanceCapacity;
}

std::uint32_t
RayTracingPartitionedTlasStrategy::ResolvePartitionCount(
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	return IsUsablePartitionPlan(partitionPlan)
	           ? partitionPlan->Counts.PartitionCount
	           : 0u;
}

std::uint32_t
RayTracingPartitionedTlasStrategy::
    ResolveMaxInstancesPerPartition(
        std::uint32_t instanceCapacity,
        const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	if (instanceCapacity == 0 ||
	    !IsUsablePartitionPlan(partitionPlan))
	{
		return 0;
	}

	return partitionPlan->Counts.MaxInstancesPerPartition;
}

bool RayTracingPartitionedTlasStrategy::
    CanUsePartitionedTlasProvider(
        const RayTracingCapabilityReport& capabilityReport) noexcept
{
	return capabilityReport.PartitionedTlas.Supported &&
	       (capabilityReport.TlasShaderAccess.SupportsDescriptor ||
	        capabilityReport.TlasShaderAccess
	            .SupportsShaderDeviceAddress);
}

const char*
RayTracingPartitionedTlasStrategy::ResolveInactiveProviderReason(
    const RayTracingCapabilityReport& capabilityReport) noexcept
{
	if (!capabilityReport.PartitionedTlas.Supported)
	{
		return capabilityReport.PartitionedTlas
		    .CapabilityStatusReason;
	}
	if (!capabilityReport.TlasShaderAccess.SupportsDescriptor &&
	    !capabilityReport.TlasShaderAccess
	         .SupportsShaderDeviceAddress)
	{
		return "partitioned-tlas-shader-binding-path-unavailable";
	}
	return "partitioned-tlas-provider-unavailable";
}

const char*
RayTracingPartitionedTlasStrategy::ResolveActiveProviderReason() noexcept
{
	return "partitioned-tlas-selected";
}

RayTracingSceneTlasShaderAccessMode
RayTracingPartitionedTlasStrategy::ResolveActiveShaderAccessMode(
    const RayTracingCapabilityReport& capabilityReport) noexcept
{
	return capabilityReport.TlasShaderAccess.SupportsDescriptor
	           ? RayTracingSceneTlasShaderAccessMode::Descriptor
	           : RayTracingSceneTlasShaderAccessMode::
	                 ShaderDeviceAddress;
}

bool RayTracingPartitionedTlasStrategy::PartitionedTlasResources::HasSceneTlas() const noexcept
{
	return Storage && StorageAddress != 0 && InstanceCount > 0 && Built;
}

RayTracingPartitionedTlasStrategy::RayTracingPartitionedTlasStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_capabilityReport(capabilityReport)
{
}

RayTracingPartitionedTlasStrategy::~RayTracingPartitionedTlasStrategy() noexcept
{
	ReleasePartitionedTlasResources();
}

const char* RayTracingPartitionedTlasStrategy::GetStrategyName() const noexcept
{
	return "PartitionedTlasStrategy";
}

ERhiRayTracingTopLevelProvider RayTracingPartitionedTlasStrategy::GetActiveProvider() const noexcept
{
	return m_partitionedResources.HasSceneTlas() ? ERhiRayTracingTopLevelProvider::PartitionedTlas
	                                             : ERhiRayTracingTopLevelProvider::None;
}

const char* RayTracingPartitionedTlasStrategy::GetActiveProviderReason() const noexcept
{
	return m_activeProviderReason;
}

RayTracingSceneFrameData RayTracingPartitionedTlasStrategy::Prepare(
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner) noexcept
{
	const RayTracingPtlasPartitionPlan* partitionPlan =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	if (!CanUseActivePartitionedTlasProvider())
	{
		m_activeProviderReason =
		    ResolveInactiveProviderReason(
		        m_capabilityReport);
		ReleasePartitionedTlasResources();
		return {};
	}

	if (!EnsurePartitionedTlasResources(sceneData, partitionPlan))
	{
		m_activeProviderReason = "partitioned-tlas-resource-setup-failed";
		return {};
	}

	m_activeProviderReason =
	    ResolveActiveProviderReason();
	return BuildPartitionedTlasFrameData(sceneData);
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	return BuildPartitionedTlas(cmd, sceneData, blasCache, scenePlanner, diagnostics);
}

bool RayTracingPartitionedTlasStrategy::HasValidSceneTlas() const noexcept
{
	return m_partitionedResources.HasSceneTlas();
}

RhiOwnedResourceHandle RayTracingPartitionedTlasStrategy::GetSceneTlasResource() const noexcept
{
	return m_partitionedResources.Storage;
}

RhiGpuVirtualAddress RayTracingPartitionedTlasStrategy::GetSceneTlasGpuAddress() const noexcept
{
	return m_partitionedResources.StorageAddress;
}

RayTracingSceneTlasShaderAccessMode RayTracingPartitionedTlasStrategy::GetSceneTlasShaderAccessMode() const noexcept
{
	return ResolveActiveShaderAccessMode(
	    m_capabilityReport);
}

std::uint32_t RayTracingPartitionedTlasStrategy::GetSceneTlasInstanceCount() const noexcept
{
	return m_partitionedResources.InstanceCount;
}

void RayTracingPartitionedTlasStrategy::Clear() noexcept
{
	ReleasePartitionedTlasResources();
	m_activeProviderReason = "partitioned-tlas-cleared";
}

bool RayTracingPartitionedTlasStrategy::CanUseActivePartitionedTlasProvider() const noexcept
{
	return m_renderHardwareInterface != nullptr &&
	       CanUsePartitionedTlasProvider(
	           m_capabilityReport);
}

bool RayTracingPartitionedTlasStrategy::EnsurePartitionedTlasResources(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	if (m_renderHardwareInterface == nullptr ||
	    sceneData.rayTracingWork.BlasInputs.empty())
	{
		ReleasePartitionedTlasResources();
		return false;
	}

	const RhiPartitionedTlasDesc layout = BuildPartitionedTlasLayout(sceneData, partitionPlan);
	if (layout.InstanceCapacity == 0 || layout.PartitionCount == 0)
	{
		ReleasePartitionedTlasResources();
		return false;
	}

	const bool layoutChanged =
	    m_partitionedResources.Layout.InstanceCapacity < layout.InstanceCapacity ||
	    m_partitionedResources.Layout.PartitionCount != layout.PartitionCount ||
	    m_partitionedResources.Layout.MaxInstancesPerPartition < layout.MaxInstancesPerPartition ||
	    m_partitionedResources.Layout.MaxOperations < layout.MaxOperations ||
	    m_partitionedResources.Layout.MaxInstancesInGlobalPartition < layout.MaxInstancesInGlobalPartition;
	if (layoutChanged)
	{
		ReleasePartitionedTlasResources();
	}

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	const RhiPartitionedTlasBuildSizes buildSizes =
	    rayTracingService.GetPartitionedTopLevelAccelerationStructureBuildSizes(layout);
	if (buildSizes.AccelerationStructureSizeInBytes == 0 || buildSizes.BuildScratchSizeInBytes == 0)
	{
		return false;
	}

	if (!m_partitionedResources.Storage)
	{
		m_partitionedResources.Storage =
		    rayTracingService.CreatePartitionedTopLevelAccelerationStructureBuffer(buildSizes, L"RayTracingPartitionedTlas");
	}
	if (!m_partitionedResources.Scratch)
	{
		m_partitionedResources.Scratch =
		    rayTracingService.CreateRayTracingScratchBuffer(buildSizes.BuildScratchSizeInBytes, L"RayTracingPartitionedTlasScratch");
	}
	if (!m_partitionedResources.Storage || !m_partitionedResources.Scratch)
	{
		ReleasePartitionedTlasResources();
		return false;
	}

	m_partitionedResources.Layout = layout;
	m_partitionedResources.StorageAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Storage);
	m_partitionedResources.ScratchAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Scratch);
	return m_partitionedResources.StorageAddress != 0 && m_partitionedResources.ScratchAddress != 0;
}

RhiPartitionedTlasDesc RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan) const noexcept
{
	const std::uint32_t instanceCapacity =
	    ResolveInstanceCapacity(sceneData);
	return RhiPartitionedTlasDesc{
	    .InstanceCapacity = instanceCapacity,
	    .PartitionCount =
	        ResolvePartitionCount(partitionPlan),
	    .MaxInstancesPerPartition =
	        ResolveMaxInstancesPerPartition(
	            instanceCapacity,
	            partitionPlan),
	    .MaxInstancesInGlobalPartition =
	        IsUsablePartitionPlan(partitionPlan)
	            ? partitionPlan->Counts.MaxInstancesInGlobalPartition
	            : 0u,
	    .MaxOperations = 1,
	    .AllowInstanceUpdates = false,
	    .AllowPartitionTranslation = false};
}

RayTracingSceneFrameData RayTracingPartitionedTlasStrategy::BuildPartitionedTlasFrameData(
    const RenderSceneData& sceneData) const noexcept
{
	RayTracingSceneFrameData frameData{};
	frameData.IsAvailable = m_partitionedResources.Storage && m_partitionedResources.StorageAddress != 0;
	frameData.TlasResource = m_partitionedResources.Storage;
	frameData.TlasGpuAddress = m_partitionedResources.StorageAddress;
	frameData.TlasShaderAccessMode = GetSceneTlasShaderAccessMode();
	frameData.EstimatedInstanceCount =
	    static_cast<std::uint32_t>(
	        sceneData.rayTracingWork
	            .PartitionedTlasBlasInputIndices.size());
	return frameData;
}

void RayTracingPartitionedTlasStrategy::InvalidatePartitionedTlasSceneState() noexcept
{
	m_partitionedResources.InstanceCount = 0;
	m_partitionedResources.Built = false;
}

void RayTracingPartitionedTlasStrategy::ReleasePartitionedTlasResources() noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		m_partitionedResources = {};
		return;
	}

	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_partitionedResources.NativeOperationData)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.NativeOperationData);
	}
	if (m_partitionedResources.Scratch)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.Scratch);
	}
	if (m_partitionedResources.Storage)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.Storage);
	}
	m_partitionedResources = {};
}

