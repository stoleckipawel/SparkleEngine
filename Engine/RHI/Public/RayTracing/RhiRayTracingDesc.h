#pragma once

#include "../Resources/RhiResourceDesc.h"

#include <array>
#include <cstdint>

enum class ERhiRayTracingTopLevelProvider : std::uint8_t
{
	None,
	ClassicTlas,
	PartitionedTlas,
};

enum class ERhiPartitionedTlasProvider : std::uint8_t
{
	None,
	VulkanNvPartitionedAccelerationStructure,
	D3D12NvapiPartitionedTlas,
	D3D12PublicDxrRtasOperations,
};

struct RhiAccelerationStructureCapabilities
{
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
	bool SupportsAccelerationStructureShaderBinding = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
	std::uint32_t ShaderGroupHandleSizeInBytes = 0;
	std::uint32_t ShaderTableAlignmentInBytes = 0;
	std::uint32_t ShaderTableRecordAlignmentInBytes = 0;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
};

struct RhiClassicTlasCapabilities
{
	bool SupportsClassicTlasBuild = false;
	bool SupportsClassicTlasUpdate = false;
	bool SupportsGpuReadableInstanceBuffer = false;
	std::uint32_t InstanceDescSizeInBytes = 0;
};

struct RhiPartitionedTlasCapabilities
{
	bool Supported = false;
	ERhiPartitionedTlasProvider Provider = ERhiPartitionedTlasProvider::None;
	bool RequiresNvidiaDevice = false;
	bool RunsOnNvidiaDevice = false;
	bool SupportsVulkanNativePartitionedAccelerationStructure = false;
	bool SupportsVulkanExtension = false;
	bool SupportsVulkanFeatureQuery = false;
	bool SupportsVulkanFunctionLoading = false;
	bool SupportsVulkanDescriptorPath = false;
	bool SupportsD3D12NvapiPartitionedTlas = false;
	bool SupportsD3D12NvapiHeaders = false;
	bool SupportsD3D12NvapiRuntime = false;
	bool SupportsD3D12DeviceInterface = false;
	bool SupportsD3D12CommandListInterface = false;
	bool SupportsD3D12PublicDxrPartitionedTlas = false;
	bool SupportsD3D12PublicDxrHeaders = false;
	bool SupportsCpuPackedOperations = false;
	bool SupportsGpuDrivenOperations = false;
	bool SupportsGpuOperationCount = false;
	bool SupportsGpuWrittenInstanceRecords = false;
	bool SupportsGpuWrittenPartitionRecords = false;
	bool SupportsPartitionTranslation = false;
	bool SupportsGlobalPartition = false;
	bool SupportsExplicitInstanceAabb = false;
	std::uint32_t MaxOperationsPerBuild = 0;
	std::uint32_t InstanceWriteDataSizeInBytes = 0;
	std::uint32_t InstanceUpdateDataSizeInBytes = 0;
	std::uint32_t PartitionWriteDataSizeInBytes = 0;
	std::uint32_t OperationDataSizeInBytes = 0;
	std::uint32_t OperationCountDataSizeInBytes = 0;
	const char* CapabilityStatusReason = "not-queried";
};

struct RhiRayTracingProviderCapabilities
{
	ERhiRayTracingTopLevelProvider SelectedTopLevelProvider = ERhiRayTracingTopLevelProvider::None;
	const char* SelectedTopLevelProviderReason = "not-queried";
};

struct RhiPartitionedTlasDesc
{
	std::uint32_t InstanceCapacity = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t MaxInstancesPerPartition = 0;
	std::uint32_t MaxInstancesInGlobalPartition = 0;
	std::uint32_t MaxOperations = 0;
	bool AllowInstanceUpdates = true;
	bool AllowPartitionTranslation = false;
	bool AllowGpuDrivenOperations = true;
};

struct RhiPartitionedTlasBuildSizes
{
	std::uint64_t AccelerationStructureSizeInBytes = 0;
	std::uint64_t BuildScratchSizeInBytes = 0;
	std::uint64_t UpdateScratchSizeInBytes = 0;
	std::uint64_t OperationInfoSizeInBytes = 0;
	std::uint64_t OperationCountSizeInBytes = 0;
	std::uint64_t InstanceWriteInfoSizeInBytes = 0;
	std::uint64_t InstanceUpdateInfoSizeInBytes = 0;
	std::uint64_t PartitionWriteInfoSizeInBytes = 0;
};

struct RhiRayTracingCapabilityGroups
{
	RhiAccelerationStructureCapabilities AccelerationStructures;
	RhiClassicTlasCapabilities ClassicTlas;
	RhiPartitionedTlasCapabilities PartitionedTlas;
	RhiRayTracingProviderCapabilities Provider;
};

struct RhiRayTracingCapabilities
{
	bool SupportsRayTracing = false;
	bool SupportsInlineRayQuery = false;
	std::uint32_t MaxTraceRecursionDepth = 0;
	std::uint32_t MaxRayPayloadSizeInBytes = 0;
	std::uint32_t MaxRayAttributeSizeInBytes = 0;
	std::uint32_t ShaderGroupHandleSizeInBytes = 0;
	std::uint32_t ShaderTableAlignmentInBytes = 0;
	std::uint32_t ShaderTableRecordAlignmentInBytes = 0;
	std::uint64_t AccelerationStructureByteAlignment = 0;
	std::uint64_t ScratchBufferByteAlignment = 0;
	std::uint32_t InstanceDescSizeInBytes = 0;
	RhiRayTracingCapabilityGroups Groups;
};

constexpr const char* RhiRayTracingTopLevelProviderToString(ERhiRayTracingTopLevelProvider provider) noexcept
{
	switch (provider)
	{
		case ERhiRayTracingTopLevelProvider::ClassicTlas:
			return "ClassicTlas";
		case ERhiRayTracingTopLevelProvider::PartitionedTlas:
			return "PartitionedTlas";
		case ERhiRayTracingTopLevelProvider::None:
		default:
			return "None";
	}
}

constexpr const char* RhiPartitionedTlasProviderToString(ERhiPartitionedTlasProvider provider) noexcept
{
	switch (provider)
	{
		case ERhiPartitionedTlasProvider::VulkanNvPartitionedAccelerationStructure:
			return "VulkanNvPartitionedAccelerationStructure";
		case ERhiPartitionedTlasProvider::D3D12NvapiPartitionedTlas:
			return "D3D12NvapiPartitionedTlas";
		case ERhiPartitionedTlasProvider::D3D12PublicDxrRtasOperations:
			return "D3D12PublicDxrRtasOperations";
		case ERhiPartitionedTlasProvider::None:
		default:
			return "None";
	}
}

enum class ERhiRayTracingAccelerationStructureType : std::uint8_t
{
	BottomLevel,
	TopLevel,
};

struct RhiRayTracingGeometryDesc
{
	RhiGpuVirtualAddress VertexBuffer = 0;
	std::uint32_t VertexStrideInBytes = 0;
	std::uint32_t VertexCount = 0;
	RhiGpuVirtualAddress IndexBuffer = 0;
	std::uint32_t IndexCount = 0;
	RhiIndexFormat IndexFormat = RhiIndexFormat::UInt32;
	bool Opaque = true;
};

struct RhiRayTracingInstanceDesc
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};

struct RhiRayTracingAccelerationStructurePrebuildInfo
{
	std::uint64_t ResultDataMaxSizeInBytes = 0;
	std::uint64_t ScratchDataSizeInBytes = 0;
	std::uint64_t UpdateScratchDataSizeInBytes = 0;
};
