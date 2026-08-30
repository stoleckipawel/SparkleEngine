#include "PCH.h"

#include "D3D12/RayTracing/D3D12NvapiRayTracingProvider.h"

#include <d3d12.h>

#if SPARKLE_RHI_WITH_D3D12_NVAPI
  #include <nvapi.h>
#endif

static const auto g_d3d12NvapiRayTracingLogger = Logging::GetOrCreateLogger("RHI.D3D12.NVAPI.RayTracing");

#if SPARKLE_RHI_WITH_D3D12_NVAPI
  #if defined(NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS_VER) \
	  && defined(NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS_VER)
	#define SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS 1
  #else
	#define SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS 0
  #endif
#else
  #define SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS 0
#endif

D3D12NvapiRayTracingProvider::D3D12NvapiRayTracingProvider() noexcept
{
#if SPARKLE_RHI_WITH_D3D12_NVAPI
	const NvAPI_Status status = NvAPI_Initialize();
	m_runtimeInitialized = status == NVAPI_OK;
	m_runtimeStatusReason = m_runtimeInitialized ? "d3d12-nvapi-runtime-initialized" : ToNvapiStatusReason(status);
#else
	m_runtimeInitialized = false;
	m_runtimeStatusReason = "d3d12-nvapi-headers-not-compiled";
#endif
}

D3D12NvapiRayTracingProvider::~D3D12NvapiRayTracingProvider() noexcept
{
#if SPARKLE_RHI_WITH_D3D12_NVAPI
	if (m_runtimeInitialized)
	{
		const NvAPI_Status status = NvAPI_Unload();
		if (status != NVAPI_OK)
		{
			SPDLOG_LOGGER_WARN(g_d3d12NvapiRayTracingLogger, "NVAPI unload status: {}", ToNvapiStatusReason(status));
		}
	}
#endif
}

std::uint64_t D3D12NvapiRayTracingProvider::AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept
{
	return alignment == 0 ? value : ((value + alignment - 1u) / alignment) * alignment;
}

const char* D3D12NvapiRayTracingProvider::ToNvapiStatusReason(int status) noexcept
{
#if SPARKLE_RHI_WITH_D3D12_NVAPI
	switch (static_cast<NvAPI_Status>(status))
	{
		case NVAPI_OK:
			return "d3d12-nvapi-ok";
		case NVAPI_LIBRARY_NOT_FOUND:
			return "d3d12-nvapi-library-not-found";
		case NVAPI_NO_IMPLEMENTATION:
			return "d3d12-nvapi-no-implementation";
		case NVAPI_API_NOT_INITIALIZED:
			return "d3d12-nvapi-api-not-initialized";
		case NVAPI_INVALID_POINTER:
			return "d3d12-nvapi-invalid-pointer";
		case NVAPI_INVALID_ARGUMENT:
			return "d3d12-nvapi-invalid-argument";
		case NVAPI_NOT_SUPPORTED:
			return "d3d12-nvapi-not-supported";
		case NVAPI_ERROR:
		default:
			return "d3d12-nvapi-error";
	}
#else
	static_cast<void>(status);
	return "d3d12-nvapi-headers-not-compiled";
#endif
}

bool D3D12NvapiRayTracingProvider::IsRuntimeInitialized() const noexcept
{
	return m_runtimeInitialized;
}

const char* D3D12NvapiRayTracingProvider::GetRuntimeStatusReason() const noexcept
{
	return m_runtimeStatusReason;
}

RhiPartitionedTlasCapabilities D3D12NvapiRayTracingProvider::QueryPartitionedTlasCapabilities(
    ID3D12Device10* device,
    bool runsOnNvidiaDevice,
    bool supportsAccelerationStructure) const noexcept
{
	RhiPartitionedTlasCapabilities capabilities{
	    .Supported = false,
	    .Provider = ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas,
	    .NvidiaDeviceOnly = true,
	    .CurrentDeviceIsNvidia = runsOnNvidiaDevice,
	    .CapabilityStatusReason = "d3d12-nvapi-ptlas-not-queried"};

	if (!supportsAccelerationStructure)
	{
		capabilities.CapabilityStatusReason = "d3d12-ray-tracing-unavailable";
		return capabilities;
	}
	if (!runsOnNvidiaDevice)
	{
		capabilities.CapabilityStatusReason = "d3d12-nvapi-ptlas-requires-nvidia-device";
		return capabilities;
	}
	if (device == nullptr)
	{
		capabilities.CapabilityStatusReason = "d3d12-device-interface-missing";
		return capabilities;
	}
#if !SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS
	capabilities.CapabilityStatusReason =
	    SPARKLE_RHI_WITH_D3D12_NVAPI ? "d3d12-nvapi-ptlas-symbols-missing" : "d3d12-nvapi-headers-not-compiled";
	return capabilities;
#else
	if (!m_runtimeInitialized)
	{
		capabilities.CapabilityStatusReason = m_runtimeStatusReason;
		return capabilities;
	}

	NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAPS nativeCaps = NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_NONE;
	const NvAPI_Status status =
	    NvAPI_D3D12_GetRaytracingCaps(device, NVAPI_D3D12_RAYTRACING_CAPS_TYPE_PARTITIONED_TLAS, &nativeCaps, sizeof(nativeCaps));
	if (status != NVAPI_OK)
	{
		capabilities.CapabilityStatusReason = ToNvapiStatusReason(status);
		return capabilities;
	}

	const bool supportsStandard =
	    (nativeCaps & NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_STANDARD) == NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_CAP_STANDARD;
	capabilities.SupportsDescriptorAccess = supportsStandard;
	capabilities.Supported = supportsStandard;
	capabilities.SupportsCpuPackedOperations = supportsStandard;
	capabilities.SupportsGpuDrivenOperations = supportsStandard;
	capabilities.SupportsGpuOperationCount = supportsStandard;
	capabilities.SupportsGpuWrittenInstanceRecords = supportsStandard;
	capabilities.SupportsGpuWrittenPartitionRecords = supportsStandard;
	capabilities.SupportsPartitionTranslation = supportsStandard;
	capabilities.SupportsGlobalPartition = supportsStandard;
	capabilities.SupportsExplicitInstanceAabb = supportsStandard;
	capabilities.MaxOperationsPerBuild = 3;
	capabilities.OperationDataSizeInBytes = static_cast<std::uint32_t>(sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP));
	capabilities.OperationCountDataSizeInBytes = static_cast<std::uint32_t>(sizeof(NvU32));
	capabilities.InstanceWriteDataSizeInBytes =
	    static_cast<std::uint32_t>(sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE));
	capabilities.InstanceUpdateDataSizeInBytes =
	    static_cast<std::uint32_t>(sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE));
	capabilities.PartitionWriteDataSizeInBytes =
	    static_cast<std::uint32_t>(sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION));
	capabilities.CapabilityStatusReason =
	    supportsStandard ? "d3d12-nvapi-ptlas-standard-supported" : "d3d12-nvapi-ptlas-standard-cap-missing";
	return capabilities;
#endif
}

RhiPartitionedTlasBuildSizes D3D12NvapiRayTracingProvider::GetPartitionedTlasBuildSizes(
    ID3D12Device10* device,
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	if (device == nullptr || desc.InstanceCapacity == 0 || desc.PartitionCount == 0 || !m_runtimeInitialized)
	{
		return {};
	}
#if !SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS
	return {};
#else
	NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_INPUTS inputs{};
	inputs.flags = static_cast<NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAGS>(
	    NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_FAST_TRACE
	    | (desc.AllowPartitionTranslation ? NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_ENABLE_PARTITION_TRANSLATION
	                                      : NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_NONE));
	inputs.instanceCount = desc.InstanceCapacity;
	inputs.maxInstancePerPartitionCount = desc.MaxInstancesPerPartition;
	inputs.partitionCount = desc.PartitionCount;
	inputs.maxInstanceInGlobalPartitionCount = desc.MaxInstancesInGlobalPartition;

	NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO nativeInfo{};
	NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS params{};
	params.version = NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS_VER;
	params.pInput = &inputs;
	params.pInfo = &nativeInfo;
	const NvAPI_Status status = NvAPI_D3D12_GetRaytracingPartitionedTlasIndirectPrebuildInfo(device, &params);
	if (status != NVAPI_OK)
	{
		SPDLOG_LOGGER_WARN(g_d3d12NvapiRayTracingLogger, "PTLAS prebuild query failed: {}", ToNvapiStatusReason(status));
		return {};
	}

	constexpr std::uint64_t scratchAlignment = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT;
	return RhiPartitionedTlasBuildSizes{
	    .AccelerationStructureSizeInBytes =
	        AlignUp(nativeInfo.resultDataMaxSizeInBytes, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT),
	    .BuildScratchSizeInBytes = AlignUp(nativeInfo.scratchDataSizeInBytes, scratchAlignment),
	    .UpdateScratchSizeInBytes = AlignUp(nativeInfo.scratchDataSizeInBytes, scratchAlignment),
	    .OperationInfoSizeInBytes =
	        sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP) * static_cast<std::uint64_t>(desc.MaxOperations),
	    .OperationCountSizeInBytes = sizeof(NvU32),
	    .InstanceWriteInfoSizeInBytes =
	        sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE) * static_cast<std::uint64_t>(desc.InstanceCapacity),
	    .InstanceUpdateInfoSizeInBytes = desc.AllowInstanceUpdates
	        ? sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE)
	            * static_cast<std::uint64_t>(desc.InstanceCapacity)
	        : 0u,
	    .PartitionWriteInfoSizeInBytes = desc.AllowPartitionTranslation
	        ? sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION)
	            * static_cast<std::uint64_t>(desc.PartitionCount + 1u)
	        : 0u};
#endif
}

bool D3D12NvapiRayTracingProvider::BuildPartitionedTlas(
    ID3D12GraphicsCommandList7* commandList,
    const RhiPartitionedTlasBuildCommandDesc& desc) const noexcept
{
	if (commandList == nullptr || !m_runtimeInitialized || desc.DestinationAccelerationStructure == 0 || desc.Scratch == 0
	    || desc.OperationHeaders == 0 || desc.OperationCount == 0)
	{
		return false;
	}
#if !SPARKLE_RHI_D3D12_NVAPI_HAS_PARTITIONED_TLAS
	return false;
#else
	NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_INPUTS inputs{};
	inputs.flags = static_cast<NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAGS>(
	    NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_FAST_TRACE
	    | (desc.Layout.AllowPartitionTranslation ? NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_ENABLE_PARTITION_TRANSLATION
	                                             : NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_FLAG_NONE));
	inputs.instanceCount = desc.Layout.InstanceCapacity;
	inputs.maxInstancePerPartitionCount = desc.Layout.MaxInstancesPerPartition;
	inputs.partitionCount = desc.Layout.PartitionCount;
	inputs.maxInstanceInGlobalPartitionCount = desc.Layout.MaxInstancesInGlobalPartition;

	NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_DESC nativeDesc{};
	nativeDesc.inputs = inputs;
	nativeDesc.srcAccelerationStructureData = desc.SourceAccelerationStructure;
	nativeDesc.destAccelerationStructureData = desc.DestinationAccelerationStructure;
	nativeDesc.scratchAccelerationStructureData = desc.Scratch;
	nativeDesc.indirectOpCount = desc.OperationCount;
	nativeDesc.indirectOps = desc.OperationHeaders;

	NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS params{};
	params.version = NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS_VER;
	params.pDesc = &nativeDesc;
	const NvAPI_Status status = NvAPI_D3D12_BuildRaytracingPartitionedTlasIndirect(commandList, &params);
	if (status != NVAPI_OK)
	{
		SPDLOG_LOGGER_WARN(g_d3d12NvapiRayTracingLogger, "PTLAS build failed: {}", ToNvapiStatusReason(status));
		return false;
	}
	return true;
#endif
}
