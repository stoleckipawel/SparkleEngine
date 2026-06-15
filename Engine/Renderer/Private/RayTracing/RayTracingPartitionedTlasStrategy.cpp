#include "PCH.h"

#include "RayTracing/RayTracingPartitionedTlasStrategy.h"

#include "Commands/RenderCommandContext.h"
#include "Meshes/GPUMesh.h"
#include "RayTracing/RayTracingBlasCache.h"
#include "RayTracing/RayTracingPerformanceDiagnostics.h"
#include "RayTracing/RayTracingPtlasLogicalUpdateStream.h"
#include "RayTracing/RayTracingPtlasOperationWriterPolicy.h"
#include "RayTracing/RayTracingPtlasPartitionPlanner.h"
#include "RayTracing/RayTracingTopLevelScenePlanner.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "SceneData/RenderSceneData.h"

#include <algorithm>
#include <array>
#include <unordered_set>
#include <vector>

static const auto g_rayTracingPartitionedTlasStrategyLogger = Logging::GetOrCreateLogger("Renderer.RayTracing");

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

	void PopulatePlannerCounters(
	    RayTracingTopLevelAccelerationStructureBuildStats& stats,
	    const RayTracingPtlasPartitionPlan* partitionPlan) noexcept
	{
		if (partitionPlan == nullptr)
		{
			return;
		}

		stats.PtlasPlanner.PartitionCount = partitionPlan->Counts.PartitionCount;
		stats.PtlasPlanner.DirtyTransformCount = partitionPlan->Counts.DirtyTransformCount;
		stats.PtlasPlanner.MovedPartitionCount = partitionPlan->Counts.MovedPartitionCount;
		stats.PtlasPlanner.GlobalPartitionInstanceCount = partitionPlan->Counts.GlobalPartitionInstanceCount;
		stats.PtlasPlanner.DuplicateStableIndexCount = partitionPlan->Counts.DuplicateStableIndexCount;
		stats.PtlasPlanner.Overflow =
		    partitionPlan->Validation.HasPartitionOverflow || partitionPlan->Validation.HasInvalidPartition;
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

	const char* ResolveActiveWriterReason(
	    const RayTracingPartitionedTlasCapabilityReport& capabilityReport,
	    ERhiPartitionedTlasOperationWriterPath writerPath) noexcept
	{
		if (writerPath == ERhiPartitionedTlasOperationWriterPath::GpuLogicalDirtyCpuNativePack)
		{
			switch (capabilityReport.Provider)
			{
				case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
					return "d3d12-nvapi-partitioned-tlas-active-gpu-logical-cpu-native-pack";
				case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
					return "vulkan-nv-partitioned-tlas-active-gpu-logical-cpu-native-pack";
				case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
					return "d3d12-public-dxr-partitioned-tlas-active-gpu-logical-cpu-native-pack";
				case ERhiPartitionedTlasProvider::None:
				default:
					return "partitioned-tlas-active-gpu-logical-cpu-native-pack";
			}
		}

		return ResolveActiveCpuPackReason(capabilityReport);
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
	return LogicalUpdateRecords && NativeOperationData && Scratch;
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
	UploadLogicalUpdateRecords(logicalUpdates, diagnostics);
}

void RayTracingPartitionedTlasStrategy::PackPartitionedTlasNativeOperations(
    RenderCommandContext& cmd,
    const RenderSceneData& sceneData,
    RayTracingTopLevelScenePlanner* scenePlanner,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
	(void)cmd;
	(void)sceneData;
	(void)scenePlanner;
	(void)diagnostics;
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
	if (!m_partitionedResources.LogicalUpdateRecords)
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

	if (!m_partitionedResources.Storage || !m_partitionedResources.Scratch || !m_partitionedResources.LogicalUpdateRecords)
	{
		ReleasePartitionedTlasResources();
		SPDLOG_LOGGER_WARN(g_rayTracingPartitionedTlasStrategyLogger, "RayTracingPartitionedTlasStrategy: resource setup failed.");
		return false;
	}

	m_partitionedResources.Layout = layout;
	m_partitionedResources.StorageAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Storage);
	m_partitionedResources.ScratchAddress = resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.Scratch);
	m_partitionedResources.InstanceCount = layout.InstanceCapacity;
	m_partitionedResources.LogicalUpdateCount = 0;
	m_partitionedResources.Built = false;
	return m_partitionedResources.StorageAddress != 0 && m_partitionedResources.ScratchAddress != 0;
}

RhiPartitionedTlasDesc RayTracingPartitionedTlasStrategy::BuildPartitionedTlasLayout(
    const RenderSceneData& sceneData,
    const RayTracingPtlasPartitionPlan* partitionPlan) const noexcept
{
	const std::uint32_t instanceCapacity = static_cast<std::uint32_t>(sceneData.meshInstances.size());
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
	const RayTracingPtlasOperationWriterPolicy writerPolicy =
	    RayTracingPtlasOperationWriterPolicyResolver::ResolveForCapability(m_capabilityReport.PartitionedTlas);
	result.PtlasGpuUpdates.RequestedWriterPath = writerPolicy.RequestedPath;
	result.PtlasGpuUpdates.SelectedWriterPath = writerPolicy.SelectedPath;
	result.PtlasGpuUpdates.WriterSelectionReason = writerPolicy.SelectionReason;
	result.PtlasGpuUpdates.GpuDrivenOperationApiSupported = m_capabilityReport.PartitionedTlas.SupportsGpuDrivenOperations;
	result.PtlasGpuUpdates.GpuLogicalUpdateWriterAvailable =
	    m_capabilityReport.PartitionedTlas.SupportsGpuLogicalUpdateRecordWrites;
	result.PtlasGpuUpdates.FullGpuNativePackAvailable = m_capabilityReport.PartitionedTlas.SupportsGpuNativeOperationPacking;

	const RayTracingPtlasPartitionPlan* partitionPlan =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentPartitionPlan() : nullptr;
	RayTracingPartitionedTlasStrategyDetails::PopulatePlannerCounters(result.Stats, partitionPlan);
	if (!EnsurePartitionedTlasResources(sceneData, partitionPlan))
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-resource-setup-failed-after-frame-prepare";
		return result;
	}
	const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates =
	    scenePlanner != nullptr ? scenePlanner->GetCurrentLogicalUpdateStream() : nullptr;
	UploadLogicalUpdateRecords(logicalUpdates, diagnostics);

	std::vector<RhiPartitionedTlasInstanceWriteDesc> instanceWrites;
	std::unordered_set<void*> builtBlasResources;
	instanceWrites.reserve(sceneData.meshInstances.size());
	{
		auto instancePrepCpuScope =
		    diagnostics != nullptr ? diagnostics->BeginTlasInstancePreparationCpuScope() : RayTracingPerformanceDiagnostics::CpuScope{};
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
			instanceWrites.push_back(
			    RhiPartitionedTlasInstanceWriteDesc{
			        .Transform = RayTracingPartitionedTlasStrategyDetails::BuildInstanceTransform(draw.Transform.WorldMatrix),
			        .ExplicitBoundingBox = {},
			        .InstanceID = renderInstanceIndex,
			        .InstanceMask = 0xFFu,
			        .InstanceContributionToHitGroupIndex = 0u,
			        .Flags = RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable,
			        .InstanceIndex = static_cast<std::uint32_t>(instanceWrites.size()),
			        .PartitionIndex = entry != nullptr ? entry->Assignment.PartitionId : 0u,
			        .AccelerationStructure = blas.gpuAddress});
		}
	}

	result.Stats.Build.InstanceCount = static_cast<std::uint32_t>(instanceWrites.size());
	if (instanceWrites.empty())
	{
		return result;
	}

	if (writerPolicy.SelectedPath == ERhiPartitionedTlasOperationWriterPath::None)
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = writerPolicy.SelectionReason;
		return result;
	}

	const RhiPartitionedTlasOperationHeader operation{
	    .Type = ERhiPartitionedTlasOperationType::WriteInstance,
	    .ArgumentCount = result.Stats.Build.InstanceCount,
	    .ArgumentData = 0,
	    .ArgumentStrideInBytes = 0};
	const RhiPartitionedTlasOperationPackDesc operationPack{
	    .Operations = &operation,
	    .OperationCount = 1,
	    .InstanceWrites = instanceWrites.data(),
	    .InstanceWriteCount = result.Stats.Build.InstanceCount,
	    .InstanceUpdates = nullptr,
	    .InstanceUpdateCount = 0,
	    .PartitionTranslations = nullptr,
	    .PartitionTranslationCount = 0};

	RhiRayTracingService& rayTracingService = m_renderHardwareInterface->GetRayTracingService();
	RhiResourceService& resourceService = m_renderHardwareInterface->GetResourceService();
	if (m_partitionedResources.NativeOperationData)
	{
		resourceService.ReleaseOwnedResource(m_partitionedResources.NativeOperationData);
		m_partitionedResources.NativeOperationData = {};
	}
	{
		auto cpuPackScope =
		    diagnostics != nullptr ? diagnostics->BeginPartitionedTlasCpuPackScope() : RayTracingPerformanceDiagnostics::CpuScope{};
		m_partitionedResources.NativeOperationData =
		    rayTracingService.CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
		        operationPack,
		        L"RayTracingPartitionedTlasCpuPackedOperations");
	}
	if (!m_partitionedResources.NativeOperationData)
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-operation-pack-failed";
		return result;
	}

	m_partitionedResources.NativeOperationLayout =
	    rayTracingService.GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(m_partitionedResources.Layout);
	m_partitionedResources.NativeOperationDataAddress =
	    resourceService.GetResourceGpuVirtualAddress(m_partitionedResources.NativeOperationData);
	m_partitionedResources.NativeOperationCount = 1;
	result.PtlasGpuUpdates.NativeOperationCount = m_partitionedResources.NativeOperationCount;
	if (logicalUpdates != nullptr)
	{
		m_partitionedResources.LogicalUpdateCount = logicalUpdates->LogicalUpdateCount;
		result.PtlasGpuUpdates.LogicalUpdateCount = logicalUpdates->LogicalUpdateCount;
		result.PtlasGpuUpdates.ValidationMismatchCount = logicalUpdates->SkippedInvalidInstanceCount;
	}
	else
	{
		result.PtlasGpuUpdates.LogicalUpdateCount = result.Stats.Build.InstanceCount;
	}
	if (m_partitionedResources.NativeOperationDataAddress == 0)
	{
		result.ActiveProvider = ERhiRayTracingTopLevelProvider::None;
		result.ActiveProviderReason = "partitioned-tlas-operation-buffer-address-missing";
		return result;
	}

	for (void* resourceValue : builtBlasResources)
	{
		cmd.UnorderedAccessBarrier(NativeResourceHandle{resourceValue});
	}

	{
		auto tlasCpuScope = diagnostics != nullptr ? diagnostics->BeginTlasCpuScope() : RayTracingPerformanceDiagnostics::CpuScope{};
		auto tlasGpuScope = diagnostics != nullptr ? diagnostics->BeginGpuEvent("Partitioned TLAS Build") : ScopedGpuEvent{};
		auto tlasGpuTimer = diagnostics != nullptr ? diagnostics->BeginGpuTimer("Partitioned TLAS Build") : ScopedGpuTimer{};
		cmd.BuildPartitionedTopLevelAccelerationStructure(
		    RhiPartitionedTlasBuildCommandDesc{
		        .Layout = m_partitionedResources.Layout,
		        .SourceAccelerationStructure = 0,
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
	m_partitionedResources.InstanceCount = result.Stats.Build.InstanceCount;
	m_partitionedResources.Built = true;
	m_activeProviderReason =
	    RayTracingPartitionedTlasStrategyDetails::ResolveActiveWriterReason(m_capabilityReport.PartitionedTlas, writerPolicy.SelectedPath);
	result.ActiveProviderReason = m_activeProviderReason;
	return result;
}

bool RayTracingPartitionedTlasStrategy::UploadLogicalUpdateRecords(
    const RayTracingPtlasLogicalUpdateStreamResult* logicalUpdates,
    RayTracingPerformanceDiagnostics* diagnostics) noexcept
{
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
	auto logicalDirtyScope =
	    diagnostics != nullptr ? diagnostics->BeginGpuEvent("RayTracing.PTLAS.LogicalDirty") : ScopedGpuEvent{};
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
