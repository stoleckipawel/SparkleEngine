#include "PCH.h"

#include "D3D12/RayTracing/D3D12PartitionedTlasServices.h"

#include "D3D12/D3D12TypeConversions.h"
#include "D3D12/Device/D3D12Rhi.h"
#include "D3D12/Memory/D3D12GpuAllocation.h"
#include "D3D12/Memory/D3D12GpuMemoryAllocator.h"
#include "D3D12/RayTracing/D3D12NvapiRayTracingProvider.h"
#include "Memory/RhiMemoryTypes.h"
#include "RayTracing/RhiPartitionedTlasOperationLayout.h"
#include "Resources/RhiResourceDesc.h"
#include "Validation/RhiContract.h"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

#if SPARKLE_RHI_WITH_D3D12_NVAPI
  #include <nvapi.h>
#endif

#if SPARKLE_RHI_WITH_D3D12_NVAPI
  #if defined(NVAPI_GET_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PREBUILD_INFO_PARAMS_VER) \
	  && defined(NVAPI_BUILD_RAYTRACING_PARTITIONED_TLAS_INDIRECT_PARAMS_VER)
	#define SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS 1
  #else
	#define SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS 0
  #endif
#else
  #define SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS 0
#endif

namespace D3D12PartitionedTlasText
{
	std::wstring MakeDebugName(std::wstring_view debugName, std::wstring_view defaultDebugName)
	{
		return debugName.empty() ? std::wstring(defaultDebugName) : std::wstring(debugName);
	}
}

std::uint32_t D3D12PartitionedTlasServices::ToNvapiPartitionedInstanceFlags(RhiPartitionedTlasInstanceFlags flags) noexcept
{
#if SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS
	std::uint32_t nativeFlags = NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_NONE;
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable))
	{
		nativeFlags |= NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::TriangleFlipFacing))
	{
		nativeFlags |= NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::ForceOpaque))
	{
		nativeFlags |= NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_FORCE_OPAQUE;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::ForceNoOpaque))
	{
		nativeFlags |= NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_FORCE_NON_OPAQUE;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::EnableExplicitBoundingBox))
	{
		nativeFlags |= NVAPI_D3D12_RAYTRACING_PARTITIONED_TLAS_INSTANCE_FLAG_ENABLE_EXPLICIT_AABB;
	}
	return nativeFlags;
#else
	static_cast<void>(flags);
	return 0;
#endif
}

std::uint32_t D3D12PartitionedTlasServices::ToNvapiPartitionedOperationType(ERhiPartitionedTlasOperationType type) noexcept
{
#if SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS
	switch (type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_UPDATE_INSTANCE;
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_WRITE_PARTITION;
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_WRITE_INSTANCE;
	}
#else
	static_cast<void>(type);
	return 0;
#endif
}

RhiPartitionedTlasNativeOperationLayout D3D12PartitionedTlasServices::GetNativeOperationLayout() noexcept
{
#if SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS
	return RhiPartitionedTlasNativeOperationLayout{
	    .OperationCountSizeInBytes = sizeof(NvU32),
	    .OperationHeaderStrideInBytes = sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP),
	    .OperationHeaderAlignmentInBytes = alignof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP),
	    .InstanceWriteStrideInBytes = sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE),
	    .InstanceWriteAlignmentInBytes = alignof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE),
	    .InstanceUpdateStrideInBytes = sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE),
	    .InstanceUpdateAlignmentInBytes = alignof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE),
	    .PartitionTranslationStrideInBytes = sizeof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION),
	    .PartitionTranslationAlignmentInBytes = alignof(NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION),
	    .BufferAlignmentInBytes = 16};
#else
	return {};
#endif
}

D3D12PartitionedTlasServices::D3D12PartitionedTlasServices(
    D3D12Rhi& rhi,
    D3D12GpuMemoryAllocator& memoryAllocator,
    D3D12NvapiRayTracingProvider& nvapiProvider) noexcept :
    m_rhi(&rhi),
    m_memoryAllocator(&memoryAllocator),
    m_nvapiProvider(&nvapiProvider)
{
}

RhiPartitionedTlasBuildSizes D3D12PartitionedTlasServices::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || m_nvapiProvider == nullptr || !capabilities.Groups.PartitionedTlas.Supported)
	{
		return {};
	}
	return m_nvapiProvider->GetPartitionedTlasBuildSizes(m_rhi->GetDevice().Get(), desc);
}

RhiOwnedResourceHandle D3D12PartitionedTlasServices::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes& sizes,
    std::wstring_view debugName)
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !capabilities.Groups.PartitionedTlas.Supported
	    || sizes.AccelerationStructureSizeInBytes == 0)
	{
		return {};
	}

	const RhiBufferResourceDesc desc{
	    .SizeInBytes = RhiPartitionedTlasOperationLayout::AlignUp(
	        sizes.AccelerationStructureSizeInBytes,
	        capabilities.AccelerationStructureByteAlignment),
	    .AllowUnorderedAccess = true};
	const D3D12_RESOURCE_DESC resourceDesc = D3D12TypeConversions::BuildBufferResourceDesc(desc);
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    D3D12PartitionedTlasText::MakeDebugName(debugName, L"RayTracingPartitionedTlasStorage"));
	return ownedRecord != nullptr ? MakeD3D12OwnedResourceHandle(std::move(ownedRecord)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle D3D12PartitionedTlasServices::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc& operationPack,
    std::wstring_view debugName)
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !capabilities.Groups.PartitionedTlas.Supported
	    || !RhiContract::IsPartitionedTlasOperationPackUsable(operationPack))
	{
		return {};
	}

#if !SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS
	return {};
#else
	const RhiPartitionedTlasNativeOperationLayout nativeLayout = GetNativeOperationLayout();
	const RhiPartitionedTlasOperationBufferLayout layout = RhiPartitionedTlasOperationLayout::Build(
	    operationPack.OperationCount,
	    operationPack.InstanceWriteCount,
	    operationPack.InstanceUpdateCount,
	    operationPack.PartitionTranslationCount,
	    nativeLayout);
	if (layout.TotalSizeInBytes == 0)
	{
		return {};
	}

	const D3D12_RESOURCE_DESC resourceDesc =
	    D3D12TypeConversions::BuildBufferResourceDesc(RhiBufferResourceDesc{.SizeInBytes = layout.TotalSizeInBytes});
	std::unique_ptr<D3D12GpuAllocationRecord> ownedRecord = m_memoryAllocator->CreateBuffer(
	    resourceDesc,
	    D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    D3D12PartitionedTlasText::MakeDebugName(debugName, L"RayTracingPartitionedTlasOperations"));
	if (ownedRecord == nullptr || ownedRecord->Resource == nullptr)
	{
		return {};
	}

	std::vector<std::uint8_t> packed(static_cast<std::size_t>(layout.TotalSizeInBytes), 0);
	std::memcpy(packed.data() + layout.OperationCountOffsetInBytes, &operationPack.OperationCount, sizeof(operationPack.OperationCount));

	auto* const nativeOperations = reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP*>(
	    packed.data() + static_cast<std::size_t>(layout.OperationHeadersOffsetInBytes));
	auto* const nativeInstanceWrites = reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE*>(
	    packed.data() + static_cast<std::size_t>(layout.InstanceWriteRecordsOffsetInBytes));
	auto* const nativeInstanceUpdates = reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE*>(
	    packed.data() + static_cast<std::size_t>(layout.InstanceUpdateRecordsOffsetInBytes));
	auto* const nativePartitionTranslations = reinterpret_cast<NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION*>(
	    packed.data() + static_cast<std::size_t>(layout.PartitionTranslationRecordsOffsetInBytes));

	const RhiGpuVirtualAddress baseAddress = ownedRecord->Resource->GetGPUVirtualAddress();
	const RhiGpuVirtualAddress instanceWriteAddress = baseAddress + layout.InstanceWriteRecordsOffsetInBytes;
	const RhiGpuVirtualAddress instanceUpdateAddress = baseAddress + layout.InstanceUpdateRecordsOffsetInBytes;
	const RhiGpuVirtualAddress partitionTranslationAddress = baseAddress + layout.PartitionTranslationRecordsOffsetInBytes;
	for (std::uint32_t operationIndex = 0; operationIndex < operationPack.OperationCount; ++operationIndex)
	{
		const RhiPartitionedTlasOperationHeader& operation = operationPack.Operations[operationIndex];
		nativeOperations[operationIndex].type =
		    static_cast<NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_TYPE>(ToNvapiPartitionedOperationType(operation.Type));
		nativeOperations[operationIndex].count = operation.ArgumentCount;
		nativeOperations[operationIndex].data.StartAddress = RhiPartitionedTlasOperationLayout::ResolveArgumentAddress(
		    operation,
		    instanceWriteAddress,
		    instanceUpdateAddress,
		    partitionTranslationAddress);
		nativeOperations[operationIndex].data.StrideInBytes =
		    RhiPartitionedTlasOperationLayout::ResolveArgumentStride(operation, nativeLayout);
	}

	for (std::uint32_t instanceIndex = 0; instanceIndex < operationPack.InstanceWriteCount; ++instanceIndex)
	{
		const RhiPartitionedTlasInstanceWriteDesc& source = operationPack.InstanceWrites[instanceIndex];
		NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_INSTANCE& target = nativeInstanceWrites[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < source.Transform.size(); ++transformIndex)
		{
			target.transform[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		std::memcpy(target.userAABB, source.ExplicitBoundingBox.data(), sizeof(target.userAABB));
		target.instanceID = source.InstanceID;
		target.instanceMask = source.InstanceMask;
		target.instanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex;
		target.instanceFlags = ToNvapiPartitionedInstanceFlags(source.Flags);
		target.instanceIndex = source.InstanceIndex;
		target.partitionIndex = source.PartitionIndex;
		target.accelerationStructure = source.AccelerationStructure;
	}

	for (std::uint32_t instanceIndex = 0; instanceIndex < operationPack.InstanceUpdateCount; ++instanceIndex)
	{
		const RhiPartitionedTlasInstanceUpdateDesc& source = operationPack.InstanceUpdates[instanceIndex];
		NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_UPDATE_INSTANCE& target = nativeInstanceUpdates[instanceIndex];
		target.instanceIndex = source.InstanceIndex;
		target.instanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex;
		target.accelerationStructure = source.AccelerationStructure;
	}

	for (std::uint32_t partitionIndex = 0; partitionIndex < operationPack.PartitionTranslationCount; ++partitionIndex)
	{
		const RhiPartitionedTlasPartitionTranslationDesc& source = operationPack.PartitionTranslations[partitionIndex];
		NVAPI_D3D12_BUILD_RAYTRACING_PARTITIONED_TLAS_OP_ARG_WRITE_PARTITION& target = nativePartitionTranslations[partitionIndex];
		target.partitionIndex = source.PartitionIndex;
		target.partitionTranslation[0] = source.Translation[0];
		target.partitionTranslation[1] = source.Translation[1];
		target.partitionTranslation[2] = source.Translation[2];
	}

	void* mappedData = nullptr;
	const D3D12_RANGE readRange{0, 0};
	if (FAILED(ownedRecord->Resource->Map(0, &readRange, &mappedData)) || mappedData == nullptr)
	{
		return {};
	}
	ownedRecord->IsMapped = true;
	ownedRecord->CpuMappedAddress = mappedData;
	std::memcpy(mappedData, packed.data(), packed.size());
	ownedRecord->Resource->Unmap(0, nullptr);
	ownedRecord->IsMapped = false;
	ownedRecord->CpuMappedAddress = nullptr;
	return MakeD3D12OwnedResourceHandle(std::move(ownedRecord));
#endif
}

RhiPartitionedTlasOperationBufferLayout D3D12PartitionedTlasServices::GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || !capabilities.Groups.PartitionedTlas.Supported || desc.MaxOperations == 0)
	{
		return {};
	}

#if !SPARKLE_RHI_D3D12_NVAPI_PACKS_PARTITIONED_TLAS
	static_cast<void>(desc);
	return {};
#else
	return RhiPartitionedTlasOperationLayout::Build(
	    desc.MaxOperations,
	    desc.InstanceCapacity,
	    desc.AllowInstanceUpdates ? desc.InstanceCapacity : 0u,
	    desc.AllowPartitionTranslation ? desc.PartitionCount + 1u : 0u,
	    GetNativeOperationLayout());
#endif
}
