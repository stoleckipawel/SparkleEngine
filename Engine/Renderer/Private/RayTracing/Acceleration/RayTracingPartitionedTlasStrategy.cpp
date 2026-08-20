#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/Preparation/PreparedRenderScene.h"

#include <algorithm>

static const auto g_rayTracingPartitionedTlasStrategyLogger = Logging::GetOrCreateLogger("Renderer.RayTracing.PartitionedTlasStrategy");

bool RayTracingPartitionedTlasStrategy::IsUsablePartitionPlan(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	return partitionPlan != nullptr && !partitionPlan->Validation.HasDuplicateStableIndices
	    && !partitionPlan->Validation.HasPartitionOverflow && partitionPlan->Counts.PartitionCount != 0;
}

std::uint32_t RayTracingPartitionedTlasStrategy::ResolveInstanceCapacity(const PreparedRenderScene& preparedScene) noexcept
{
	std::uint32_t instanceCapacity = 0;
	const RenderRayTracingWorkPlan& work = preparedScene.rayTracingWork;
	for (const std::uint32_t blasInputIndex : work.PartitionedTlasBlasInputIndices)
	{
		if (blasInputIndex >= work.BlasInputs.size())
		{
			Diagnostics::Fatal(
			    g_rayTracingPartitionedTlasStrategyLogger,
			    __FILE__,
			    __LINE__,
			    "Partitioned TLAS capacity calculation references a BLAS input outside the prepared work plan.");
		}
		const RenderRayTracingBlasInput& input = work.BlasInputs[blasInputIndex];
		instanceCapacity = (std::max) (instanceCapacity, input.GpuSceneSlot + 1u);
	}
	return instanceCapacity;
}

std::uint32_t RayTracingPartitionedTlasStrategy::ResolvePartitionCount(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	return IsUsablePartitionPlan(partitionPlan) ? partitionPlan->Counts.PartitionCount : 0u;
}

std::uint32_t RayTracingPartitionedTlasStrategy::ResolveMaxInstancesPerPartition(
    std::uint32_t instanceCapacity,
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	if (instanceCapacity == 0 || !IsUsablePartitionPlan(partitionPlan))
	{
		return 0;
	}

	return partitionPlan->Counts.MaxInstancesPerPartition;
}

bool RayTracingPartitionedTlasStrategy::CanUsePartitionedTlasProvider(const RayTracingCapabilityReport& capabilityReport) noexcept
{
	return capabilityReport.PartitionedTlas.Supported
	    && (capabilityReport.TlasShaderAccess.SupportsDescriptor || capabilityReport.TlasShaderAccess.SupportsShaderDeviceAddress);
}

const char* RayTracingPartitionedTlasStrategy::ResolveInactiveProviderReason(const RayTracingCapabilityReport& capabilityReport) noexcept
{
	if (!capabilityReport.PartitionedTlas.Supported)
	{
		return capabilityReport.PartitionedTlas.CapabilityStatusReason;
	}
	if (!capabilityReport.TlasShaderAccess.SupportsDescriptor && !capabilityReport.TlasShaderAccess.SupportsShaderDeviceAddress)
	{
		return "partitioned-tlas-shader-binding-path-unavailable";
	}
	return "partitioned-tlas-provider-unavailable";
}

const char* RayTracingPartitionedTlasStrategy::ResolveActiveProviderReason() noexcept
{
	return "partitioned-tlas-selected";
}

RayTracingSceneTlasShaderAccessMode RayTracingPartitionedTlasStrategy::ResolveActiveShaderAccessMode(
    const RayTracingCapabilityReport& capabilityReport) noexcept
{
	return capabilityReport.TlasShaderAccess.SupportsDescriptor ? RayTracingSceneTlasShaderAccessMode::Descriptor
	                                                            : RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress;
}

bool RayTracingPartitionedTlasStrategy::PartitionedTlasResources::HasSceneTlas() const noexcept
{
	return Storage && StorageAddress != 0 && Built;
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
	return m_partitionedResources.HasSceneTlas() ? ERhiRayTracingTopLevelProvider::PartitionedTlas : ERhiRayTracingTopLevelProvider::None;
}

const char* RayTracingPartitionedTlasStrategy::GetActiveProviderReason() const noexcept
{
	return m_activeProviderReason;
}

RenderRayTracingFrameBindings RayTracingPartitionedTlasStrategy::Prepare(
    const PreparedRenderScene& preparedScene,
    RayTracingTopLevelScenePlanner* scenePlanner) noexcept
{
	const RayTracingPtlasPartitionPlan* partitionPlan = scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	if (!CanUseActivePartitionedTlasProvider())
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS strategy was selected without a usable device provider.");
	}

	EnsurePartitionedTlasResources(preparedScene, partitionPlan);
	m_activeProviderReason = ResolveActiveProviderReason();
	return BuildPartitionedTlasFrameData(preparedScene);
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::Build(
    RenderCommandContext& commandContext,
    const PreparedRenderScene& preparedScene,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	return BuildPartitionedTlas(commandContext, preparedScene, blasCache, scenePlanner, diagnostics);
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
	return ResolveActiveShaderAccessMode(m_capabilityReport);
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
	return m_renderHardwareInterface != nullptr && CanUsePartitionedTlasProvider(m_capabilityReport);
}

void RayTracingPartitionedTlasStrategy::EnsurePartitionedTlasResources(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS resource allocation has no render hardware interface.");
	}

	const RhiPartitionedTlasDesc layout = BuildPartitionedTlasLayout(preparedScene, partitionPlan);
	if (layout.InstanceCapacity == 0 || layout.PartitionCount == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS planner produced an unusable resource layout.");
	}

	const bool layoutChanged = m_partitionedResources.Layout.InstanceCapacity < layout.InstanceCapacity
	    || m_partitionedResources.Layout.PartitionCount != layout.PartitionCount
	    || m_partitionedResources.Layout.MaxInstancesPerPartition < layout.MaxInstancesPerPartition
	    || m_partitionedResources.Layout.MaxOperations < layout.MaxOperations
	    || m_partitionedResources.Layout.MaxInstancesInGlobalPartition < layout.MaxInstancesInGlobalPartition;
	if (layoutChanged)
	{
		ReleasePartitionedTlasResources();
	}

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	const RhiPartitionedTlasBuildSizes buildSizes = rayTracingService.GetPartitionedTopLevelAccelerationStructureBuildSizes(layout);
	if (buildSizes.AccelerationStructureSizeInBytes == 0 || buildSizes.BuildScratchSizeInBytes == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS build sizing produced zero-sized storage.");
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
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS storage or scratch allocation failed.");
	}

	m_partitionedResources.Layout = layout;
	m_partitionedResources.StorageAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Storage);
	m_partitionedResources.ScratchAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Scratch);
	if (m_partitionedResources.StorageAddress == 0 || m_partitionedResources.ScratchAddress == 0)
	{
		Diagnostics::Fatal(
		    g_rayTracingPartitionedTlasStrategyLogger,
		    __FILE__,
		    __LINE__,
		    "Partitioned TLAS storage or scratch has no GPU address.");
	}
}

RhiPartitionedTlasDesc RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout(
    const PreparedRenderScene& preparedScene,
    const RayTracingPtlasPartitionPlan* partitionPlan) const noexcept
{
	const std::uint32_t instanceCapacity = ResolveInstanceCapacity(preparedScene);
	if (preparedScene.rayTracingWork.BlasInputs.empty())
	{
		return RhiPartitionedTlasDesc{
		    .InstanceCapacity = 1,
		    .PartitionCount = 1,
		    .MaxInstancesPerPartition = 1,
		    .MaxInstancesInGlobalPartition = 0,
		    .MaxOperations = 1,
		    .AllowInstanceUpdates = false,
		    .AllowPartitionTranslation = false};
	}
	return RhiPartitionedTlasDesc{
	    .InstanceCapacity = instanceCapacity,
	    .PartitionCount = ResolvePartitionCount(partitionPlan),
	    .MaxInstancesPerPartition = ResolveMaxInstancesPerPartition(instanceCapacity, partitionPlan),
	    .MaxInstancesInGlobalPartition = IsUsablePartitionPlan(partitionPlan) ? partitionPlan->Counts.MaxInstancesInGlobalPartition : 0u,
	    .MaxOperations = 1,
	    .AllowInstanceUpdates = false,
	    .AllowPartitionTranslation = false};
}

RenderRayTracingFrameBindings RayTracingPartitionedTlasStrategy::BuildPartitionedTlasFrameData(
    const PreparedRenderScene& preparedScene) const noexcept
{
	RenderRayTracingFrameBindings frameBindings{};
	frameBindings.TlasResource = m_partitionedResources.Storage;
	frameBindings.TlasGpuAddress = m_partitionedResources.StorageAddress;
	frameBindings.TlasShaderAccessMode = GetSceneTlasShaderAccessMode();
	frameBindings.EstimatedInstanceCount = static_cast<std::uint32_t>(preparedScene.rayTracingWork.PartitionedTlasBlasInputIndices.size());
	return frameBindings;
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
