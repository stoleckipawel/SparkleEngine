#include "PCH.h"

#include "RayTracing/Acceleration/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "RayTracing/Diagnostics/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/Acceleration/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/Acceleration/RayTracingPtlasOperationWriterPolicy.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/Acceleration/RayTracingTopLevelScenePlanner.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>

static const auto g_rayTracingPartitionedTlasStrategyLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

namespace RayTracingPartitionedTlasStrategyDetails
{
	std::uint32_t ResolveInstanceCapacity(const RenderSceneData& sceneData) noexcept
	{
		std::uint32_t instanceCapacity = 0;
		for (const MeshDraw& draw : sceneData.meshInstances)
		{
			instanceCapacity = (std::max)(instanceCapacity, draw.Source.SourceInstanceIndex + 1u);
		}
		return instanceCapacity;
	}

	std::uint32_t ResolvePartitionCount(const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
	{
		if (partitionPlan == nullptr || partitionPlan->Counts.PartitionCount == 0)
		{
			return 1;
		}

		return partitionPlan->Counts.PartitionCount;
	}

	std::uint32_t ResolveMaxInstancesPerPartition(
	    std::uint32_t instanceCapacity,
	    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
	{
		if (instanceCapacity == 0)
		{
			return 0;
		}
		if (partitionPlan == nullptr || partitionPlan->Counts.PartitionCount == 0)
		{
			return instanceCapacity;
		}

		return (std::max)(1u, instanceCapacity);
	}

	bool CanUseD3D12NvapiPartitionedTlasProvider(const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		return capabilityReport.Supported &&
		       capabilityReport.Provider == ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas &&
		       capabilityReport.SupportsD3D12NvapiProvider &&
		       capabilityReport.SupportsD3D12NvapiHeaders &&
		       capabilityReport.SupportsD3D12NvapiRuntime &&
		       capabilityReport.SupportsD3D12DeviceInterface &&
		       capabilityReport.SupportsD3D12CommandListInterface;
	}

	bool CanUseVulkanPartitionedTlasProvider(const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		return capabilityReport.Supported &&
		       capabilityReport.Provider == ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure &&
		       (capabilityReport.SupportsVulkanDescriptorPath || capabilityReport.SupportsVulkanShaderDeviceAddressPath);
	}

	const char* ResolveInactiveProviderReason(const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		if (!capabilityReport.Supported)
		{
			return capabilityReport.CapabilityStatusReason;
		}

		switch (capabilityReport.Provider)
		{
			case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
				return capabilityReport.SupportsVulkanDescriptorPath || capabilityReport.SupportsVulkanShaderDeviceAddressPath
				           ? "vulkan-nv-partitioned-tlas-provider-not-active-for-this-backend-stage"
				           : "vulkan-nv-partitioned-tlas-shader-binding-path-unavailable";
			case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
				if (!capabilityReport.SupportsD3D12NvapiHeaders)
				{
					return "d3d12-nvapi-headers-not-compiled";
				}
				if (!capabilityReport.SupportsD3D12NvapiRuntime)
				{
					return "d3d12-nvapi-runtime-unavailable";
				}
				if (!capabilityReport.SupportsD3D12DeviceInterface)
				{
					return "d3d12-device-interface-missing";
				}
				if (!capabilityReport.SupportsD3D12CommandListInterface)
				{
					return "d3d12-command-list-interface-missing";
				}
				return "d3d12-nvapi-partitioned-tlas-provider-not-active-for-this-backend-stage";
			case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
				return "d3d12-public-dxr-ptlas-provider-not-implemented";
			case ERhiPartitionedTlasProvider::None:
			default:
				return "partitioned-tlas-provider-none";
		}
	}

	const char* ResolveActiveProviderReason(const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		switch (capabilityReport.Provider)
		{
			case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
				return "d3d12-nvapi-partitioned-tlas-selected";
			case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
				return "vulkan-nv-partitioned-tlas-selected";
			case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
				return "d3d12-public-dxr-partitioned-tlas-selected";
			case ERhiPartitionedTlasProvider::None:
			default:
				return "partitioned-tlas-selected";
		}
	}

	RayTracingSceneTlasShaderAccessMode ResolveActiveShaderAccessMode(
	    const RayTracingPartitionedTlasCapabilityReport& capabilityReport) noexcept
	{
		if (capabilityReport.Provider == ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas)
		{
			return RayTracingSceneTlasShaderAccessMode::Descriptor;
		}
		return capabilityReport.SupportsVulkanDescriptorPath ? RayTracingSceneTlasShaderAccessMode::Descriptor
		                                                     : RayTracingSceneTlasShaderAccessMode::ShaderDeviceAddress;
	}

	constexpr RayTracingSceneTlasShaderAccessMode ClassicFallbackShaderAccessMode() noexcept
	{
	    return RayTracingSceneTlasShaderAccessMode::Descriptor;
	}
}

bool RayTracingPartitionedTlasStrategy::PartitionedTlasResources::HasSceneTlas() const noexcept
{
	return Storage && StorageAddress != 0 && InstanceCount > 0 && Built;
}

bool RayTracingPartitionedTlasStrategy::PartitionedTlasResources::HasFrameGraphOperationResources() const noexcept
{
	return NativeOperationData && Scratch;
}

RayTracingPartitionedTlasStrategy::RayTracingPartitionedTlasStrategy(
    RenderHardwareInterface& renderHardwareInterface,
    const RayTracingCapabilityReport& capabilityReport) noexcept :
    m_renderHardwareInterface(&renderHardwareInterface),
    m_capabilityReport(capabilityReport),
    m_classicFallbackStrategy(
        renderHardwareInterface,
        RayTracingPartitionedTlasStrategyDetails::ClassicFallbackShaderAccessMode())
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
	return m_currentFrameMode == FrameMode::PartitionedTlas ? ERhiRayTracingTopLevelProvider::PartitionedTlas
	                                                        : ERhiRayTracingTopLevelProvider::ClassicTlas;
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
		m_currentFrameMode = FrameMode::ClassicFallback;
		m_activeProviderReason = RayTracingPartitionedTlasStrategyDetails::ResolveInactiveProviderReason(m_capabilityReport.PartitionedTlas);
		return m_classicFallbackStrategy.Prepare(sceneData, scenePlanner);
	}

	if (!EnsurePartitionedTlasResources(sceneData, partitionPlan))
	{
		m_currentFrameMode = FrameMode::ClassicFallback;
		m_activeProviderReason = "partitioned-tlas-resource-setup-failed-classic-fallback";
		return m_classicFallbackStrategy.Prepare(sceneData, scenePlanner);
	}

	m_currentFrameMode = FrameMode::PartitionedTlas;
	m_activeProviderReason = RayTracingPartitionedTlasStrategyDetails::ResolveActiveProviderReason(m_capabilityReport.PartitionedTlas);
	return BuildPartitionedTlasFrameData(sceneData);
}

RayTracingTopLevelAccelerationStructureBuildResult RayTracingPartitionedTlasStrategy::Build(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingBlasCache& blasCache,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	if (m_currentFrameMode == FrameMode::PartitionedTlas)
	{
		return BuildPartitionedTlas(cmd, sceneData, blasCache, scenePlanner, diagnostics);
	}

	RayTracingTopLevelAccelerationStructureBuildResult result =
	    m_classicFallbackStrategy.Build(cmd, sceneData, blasCache, scenePlanner, diagnostics);
	result.ActiveProvider = GetActiveProvider();
	result.ActiveProviderReason = GetActiveProviderReason();
	return result;
}

void RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLogicalUpdateResources(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)cmd;
	(void)sceneData;
	if (m_currentFrameMode != FrameMode::PartitionedTlas)
	{
		return;
	}

	const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentLogicalUpdateStream() : nullptr;
	if (RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(m_capabilityReport.PartitionedTlas).SelectedPath !=
	    ERhiPartitionedTlasOperationWriterPath::CpuPack)
	{
		UploadLogicalUpdateRecords(logicalUpdates, diagnostics);
	}
}

void RayTracingPartitionedTlasStrategy::PackPartitionedTlasNativeOperations(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)cmd;
	(void)sceneData;
	if (m_currentFrameMode != FrameMode::PartitionedTlas)
	{
		return;
	}

	// CPU native packing is performed during BuildPartitionedTlas, where BLAS addresses are available.
	// This pass only maintains GPU-side logical-update inputs for future GPU pack paths.
	const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentLogicalUpdateStream() : nullptr;
	if (RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(m_capabilityReport.PartitionedTlas).SelectedPath !=
	    ERhiPartitionedTlasOperationWriterPath::CpuPack)
	{
		UploadLogicalUpdateRecords(logicalUpdates, diagnostics);
	}
}

bool RayTracingPartitionedTlasStrategy::HasValidSceneTlas() const noexcept
{
	return m_currentFrameMode == FrameMode::PartitionedTlas ? m_partitionedResources.HasSceneTlas()
	                                                        : m_classicFallbackStrategy.HasValidSceneTlas();
}

RhiOwnedResourceHandle RayTracingPartitionedTlasStrategy::GetSceneTlasResource() const noexcept
{
	return m_currentFrameMode == FrameMode::PartitionedTlas ? m_partitionedResources.Storage
	                                                        : m_classicFallbackStrategy.GetSceneTlasResource();
}

RhiGpuVirtualAddress RayTracingPartitionedTlasStrategy::GetSceneTlasGpuAddress() const noexcept
{
	return m_currentFrameMode == FrameMode::PartitionedTlas ? m_partitionedResources.StorageAddress
	                                                        : m_classicFallbackStrategy.GetSceneTlasGpuAddress();
}

RayTracingSceneTlasShaderAccessMode RayTracingPartitionedTlasStrategy::GetSceneTlasShaderAccessMode() const noexcept
{
	if (m_currentFrameMode != FrameMode::PartitionedTlas)
	{
		return m_classicFallbackStrategy.GetSceneTlasShaderAccessMode();
	}
	return RayTracingPartitionedTlasStrategyDetails::ResolveActiveShaderAccessMode(m_capabilityReport.PartitionedTlas);
}

std::uint32_t RayTracingPartitionedTlasStrategy::GetSceneTlasInstanceCount() const noexcept
{
	return m_currentFrameMode == FrameMode::PartitionedTlas ? m_partitionedResources.InstanceCount
	                                                        : m_classicFallbackStrategy.GetSceneTlasInstanceCount();
}

void RayTracingPartitionedTlasStrategy::Clear() noexcept
{
	ReleasePartitionedTlasResources();
	m_classicFallbackStrategy.Clear();
	m_currentFrameMode = FrameMode::ClassicFallback;
	m_activeProviderReason = "partitioned-tlas-cleared";
}

bool RayTracingPartitionedTlasStrategy::CanUseActivePartitionedTlasProvider() const noexcept
{
	return m_renderHardwareInterface != nullptr &&
	       (RayTracingPartitionedTlasStrategyDetails::CanUseVulkanPartitionedTlasProvider(m_capabilityReport.PartitionedTlas) ||
	        RayTracingPartitionedTlasStrategyDetails::CanUseD3D12NvapiPartitionedTlasProvider(m_capabilityReport.PartitionedTlas));
}

bool RayTracingPartitionedTlasStrategy::EnsurePartitionedTlasResources(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
{
	if (m_renderHardwareInterface == nullptr || sceneData.meshInstances.empty())
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
	const RayTracingPtlasOperationWriterPolicy writerPolicy =
	    RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(m_capabilityReport.PartitionedTlas);
	const bool requiresLogicalUpdateRecords = writerPolicy.SelectedPath != ERhiPartitionedTlasOperationWriterPath::CpuPack;
	if (requiresLogicalUpdateRecords && !m_partitionedResources.LogicalUpdateRecords)
	{
		m_partitionedResources.LogicalUpdateRecords =
		    rayTracingService.CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
		        RhiPartitionedTlasLogicalUpdateBufferDesc{
		            .MaxLogicalUpdateCount = layout.InstanceCapacity,
		            .AllowGpuWrites = true,
		            .AllowCpuUploadReference = true},
		        nullptr,
		        0,
		        L"RayTracingPartitionedTlasLogicalUpdates");
	}

	if (!m_partitionedResources.Storage || !m_partitionedResources.Scratch ||
	    (requiresLogicalUpdateRecords && !m_partitionedResources.LogicalUpdateRecords))
	{
		ReleasePartitionedTlasResources();
		SPDLOG_LOGGER_WARN(g_rayTracingPartitionedTlasStrategyLogger, "RayTracingPartitionedTlasStrategy: resource setup failed.");
		return false;
	}

	m_partitionedResources.Layout = layout;
	m_partitionedResources.StorageAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Storage);
	m_partitionedResources.ScratchAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Scratch);
	m_partitionedResources.LogicalUpdateCount = 0;
	m_partitionedResources.NativeOperationCount = 0;
	return m_partitionedResources.StorageAddress != 0 && m_partitionedResources.ScratchAddress != 0;
}

RhiPartitionedTlasDesc RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan) const noexcept
{
	const std::uint32_t instanceCapacity = RayTracingPartitionedTlasStrategyDetails::ResolveInstanceCapacity(sceneData);
	const RayTracingPtlasOperationWriterPolicy writerPolicy =
	    RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(m_capabilityReport.PartitionedTlas);
	return RhiPartitionedTlasDesc{
	    .InstanceCapacity = instanceCapacity,
	    .PartitionCount = RayTracingPartitionedTlasStrategyDetails::ResolvePartitionCount(partitionPlan),
	    .MaxInstancesPerPartition =
	        RayTracingPartitionedTlasStrategyDetails::ResolveMaxInstancesPerPartition(instanceCapacity, partitionPlan),
	    .MaxInstancesInGlobalPartition =
	        partitionPlan != nullptr ? (std::max)(1u, partitionPlan->Counts.GlobalPartitionInstanceCount) : instanceCapacity,
	    .MaxOperations = 1,
	    .AllowInstanceUpdates = true,
	    .AllowPartitionTranslation = false,
	    .AllowGpuDrivenOperations = writerPolicy.SelectedPath != ERhiPartitionedTlasOperationWriterPath::CpuPack};
}

RayTracingSceneFrameData RayTracingPartitionedTlasStrategy::BuildPartitionedTlasFrameData(
    const RenderSceneData& sceneData) const noexcept
{
	RayTracingSceneFrameData frameData{};
	frameData.IsAvailable = m_partitionedResources.Storage && m_partitionedResources.StorageAddress != 0;
	frameData.TlasResource = m_partitionedResources.Storage;
	frameData.TlasGpuAddress = m_partitionedResources.StorageAddress;
	frameData.TlasShaderAccessMode = GetSceneTlasShaderAccessMode();
	frameData.EstimatedInstanceCount = static_cast<std::uint32_t>(sceneData.meshInstances.size());
	return frameData;
}

bool RayTracingPartitionedTlasStrategy::UploadLogicalUpdateRecords(
    const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)diagnostics;
	if (m_renderHardwareInterface == nullptr || m_partitionedResources.Layout.InstanceCapacity == 0)
	{
		return false;
	}

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_partitionedResources.LogicalUpdateRecords)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.LogicalUpdateRecords);
		m_partitionedResources.LogicalUpdateRecords = {};
	}

	const std::uint32_t logicalUpdateCount = logicalUpdates != nullptr ? logicalUpdates->LogicalUpdateCount : 0;
	const RhiPartitionedTlasLogicalUpdateRecord* records =
	    logicalUpdateCount > 0 && logicalUpdates != nullptr ? logicalUpdates->Records.data() : nullptr;
	m_partitionedResources.LogicalUpdateRecords =
	    rayTracingService.CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	        RhiPartitionedTlasLogicalUpdateBufferDesc{
	            .MaxLogicalUpdateCount = m_partitionedResources.Layout.InstanceCapacity,
	            .AllowGpuWrites = true,
	            .AllowCpuUploadReference = true},
	        records,
	        logicalUpdateCount,
	        L"RayTracingPartitionedTlasLogicalUpdates");
	m_partitionedResources.LogicalUpdateCount = m_partitionedResources.LogicalUpdateRecords ? logicalUpdateCount : 0;
	return static_cast<bool>(m_partitionedResources.LogicalUpdateRecords);
}

void RayTracingPartitionedTlasStrategy::InvalidatePartitionedTlasSceneState() noexcept
{
	m_partitionedResources.InstanceCount = 0;
	m_partitionedResources.LogicalUpdateCount = 0;
	m_partitionedResources.NativeOperationCount = 0;
	m_partitionedResources.StableInstanceFingerprint = 0;
	m_partitionedResources.IncrementalUpdatesAllowed = false;
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
	if (m_partitionedResources.LogicalUpdateRecords)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.LogicalUpdateRecords);
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

