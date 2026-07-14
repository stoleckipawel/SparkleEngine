#pragma once

#include "RhiAccelerationStructureDesc.h"
#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"

#include <array>
#include <cstdint>

enum class ERhiPartitionedTlasProvider : std::uint8_t
{
	None,
	VulkanNvPartitionedAccelerationStructure,
	D3D12NvapiPartitionedTlas,
	D3D12PublicDxrRtasOperations,
};

struct RhiPartitionedTlasCapabilities
{
	bool Supported = false;
	ERhiPartitionedTlasProvider Provider = ERhiPartitionedTlasProvider::None;
	bool RequiresNvidiaDevice = false;
	bool RunsOnNvidiaDevice = false;
	bool SupportsDescriptorAccess = false;
	bool SupportsShaderDeviceAddressAccess = false;
	bool SupportsVulkanNativePartitionedAccelerationStructure = false;
	bool SupportsVulkanExtension = false;
	bool SupportsVulkanFeatureQuery = false;
	bool SupportsVulkanFunctionLoading = false;
	bool SupportsVulkanDescriptorPath = false;
	bool SupportsVulkanShaderDeviceAddressPath = false;
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

struct RhiPartitionedTlasDesc
{
	std::uint32_t InstanceCapacity = 0;
	std::uint32_t PartitionCount = 0;
	std::uint32_t MaxInstancesPerPartition = 0;
	std::uint32_t MaxInstancesInGlobalPartition = 0;
	std::uint32_t MaxOperations = 0;
	bool AllowInstanceUpdates = false;
	bool AllowPartitionTranslation = false;
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

enum class ERhiPartitionedTlasOperationType : std::uint8_t
{
	WriteInstance,
	UpdateInstance,
	WritePartitionTranslation,
};

enum class RhiPartitionedTlasInstanceFlags : std::uint32_t
{
	None = 0,
	TriangleFacingCullDisable = 1u << 0u,
	TriangleFlipFacing = 1u << 1u,
	ForceOpaque = 1u << 2u,
	ForceNoOpaque = 1u << 3u,
	EnableExplicitBoundingBox = 1u << 4u,
};

SPARKLE_RHI_API RhiPartitionedTlasInstanceFlags operator|(
    RhiPartitionedTlasInstanceFlags lhs,
    RhiPartitionedTlasInstanceFlags rhs) noexcept;
SPARKLE_RHI_API bool HasFlag(RhiPartitionedTlasInstanceFlags flags, RhiPartitionedTlasInstanceFlags flag) noexcept;

struct RhiPartitionedTlasOperationHeader
{
	ERhiPartitionedTlasOperationType Type = ERhiPartitionedTlasOperationType::WriteInstance;
	std::uint32_t ArgumentCount = 0;
	RhiGpuVirtualAddress ArgumentData = 0;
	std::uint64_t ArgumentStrideInBytes = 0;
};

struct RhiPartitionedTlasInstanceWriteDesc
{
	std::array<float, 12> Transform = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f};
	std::array<float, 6> ExplicitBoundingBox = {};
	std::uint32_t InstanceID = 0;
	std::uint32_t InstanceMask = 0xFF;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiPartitionedTlasInstanceFlags Flags = RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable;
	std::uint32_t InstanceIndex = 0;
	std::uint32_t PartitionIndex = 0;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};

struct RhiPartitionedTlasInstanceUpdateDesc
{
	std::uint32_t InstanceIndex = 0;
	std::uint32_t InstanceContributionToHitGroupIndex = 0;
	RhiGpuVirtualAddress AccelerationStructure = 0;
};

struct RhiPartitionedTlasPartitionTranslationDesc
{
	std::uint32_t PartitionIndex = 0;
	std::array<float, 3> Translation = {};
};

struct RhiPartitionedTlasOperationBufferLayout
{
	std::uint64_t OperationCountOffsetInBytes = 0;
	std::uint64_t OperationHeadersOffsetInBytes = 0;
	std::uint64_t InstanceWriteRecordsOffsetInBytes = 0;
	std::uint64_t InstanceUpdateRecordsOffsetInBytes = 0;
	std::uint64_t PartitionTranslationRecordsOffsetInBytes = 0;
	std::uint64_t TotalSizeInBytes = 0;
	std::uint64_t OperationHeaderStrideInBytes = 0;
	std::uint64_t InstanceWriteStrideInBytes = 0;
	std::uint64_t InstanceUpdateStrideInBytes = 0;
	std::uint64_t PartitionTranslationStrideInBytes = 0;
};

struct RhiPartitionedTlasOperationPackDesc
{
	const RhiPartitionedTlasOperationHeader* Operations = nullptr;
	std::uint32_t OperationCount = 0;
	const RhiPartitionedTlasInstanceWriteDesc* InstanceWrites = nullptr;
	std::uint32_t InstanceWriteCount = 0;
	const RhiPartitionedTlasInstanceUpdateDesc* InstanceUpdates = nullptr;
	std::uint32_t InstanceUpdateCount = 0;
	const RhiPartitionedTlasPartitionTranslationDesc* PartitionTranslations = nullptr;
	std::uint32_t PartitionTranslationCount = 0;
};

struct RhiPartitionedTlasBuildCommandDesc
{
	RhiPartitionedTlasDesc Layout;
	RhiGpuVirtualAddress SourceAccelerationStructure = 0;
	RhiGpuVirtualAddress DestinationAccelerationStructure = 0;
	RhiGpuVirtualAddress Scratch = 0;
	RhiGpuVirtualAddress OperationHeaders = 0;
	RhiGpuVirtualAddress OperationCount = 0;
};

SPARKLE_RHI_API const char* RhiPartitionedTlasProviderToString(ERhiPartitionedTlasProvider provider) noexcept;
