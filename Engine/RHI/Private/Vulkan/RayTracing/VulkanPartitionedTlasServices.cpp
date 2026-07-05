#include "Vulkan/VulkanPCH.h"

#include "Vulkan/RayTracing/VulkanPartitionedTlasServices.h"

#include "Memory/RhiMemoryTypes.h"
#include "RayTracing/RhiRayTracingDesc.h"
#include "Resources/RhiResourceDesc.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <cstring>
#include <memory>
#include <vector>

std::uint64_t VulkanPartitionedTlasServices::AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept
{
	return alignment == 0 ? value : ((value + alignment - 1u) / alignment) * alignment;
}

VkPartitionedAccelerationStructureInstanceFlagsNV VulkanPartitionedTlasServices::ToVkPartitionedInstanceFlags(
    RhiPartitionedTlasInstanceFlags flags) noexcept
{
	VkPartitionedAccelerationStructureInstanceFlagsNV nativeFlags = 0;
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::TriangleFacingCullDisable))
	{
		nativeFlags |= VK_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCE_FLAG_TRIANGLE_FACING_CULL_DISABLE_BIT_NV;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::TriangleFlipFacing))
	{
		nativeFlags |= VK_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCE_FLAG_TRIANGLE_FLIP_FACING_BIT_NV;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::ForceOpaque))
	{
		nativeFlags |= VK_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCE_FLAG_FORCE_OPAQUE_BIT_NV;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::ForceNoOpaque))
	{
		nativeFlags |= VK_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCE_FLAG_FORCE_NO_OPAQUE_BIT_NV;
	}
	if (HasFlag(flags, RhiPartitionedTlasInstanceFlags::EnableExplicitBoundingBox))
	{
		nativeFlags |= VK_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCE_FLAG_ENABLE_EXPLICIT_BOUNDING_BOX_NV;
	}
	return nativeFlags;
}

VkPartitionedAccelerationStructureOpTypeNV VulkanPartitionedTlasServices::ToVkPartitionedOperationType(
    ERhiPartitionedTlasOperationType type) noexcept
{
	switch (type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return VK_PARTITIONED_ACCELERATION_STRUCTURE_OP_TYPE_UPDATE_INSTANCE_NV;
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return VK_PARTITIONED_ACCELERATION_STRUCTURE_OP_TYPE_WRITE_PARTITION_TRANSLATION_NV;
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return VK_PARTITIONED_ACCELERATION_STRUCTURE_OP_TYPE_WRITE_INSTANCE_NV;
	}
}

void VulkanPartitionedTlasServices::ConfigurePartitionedTlasInput(
    const RhiPartitionedTlasDesc& desc,
    VkPartitionedAccelerationStructureInstancesInputNV& input,
    VkPartitionedAccelerationStructureFlagsNV& flags) noexcept
{
	flags = VkPartitionedAccelerationStructureFlagsNV{
	    .sType = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_FLAGS_NV,
	    .pNext = nullptr,
	    .enablePartitionTranslation = desc.AllowPartitionTranslation ? VK_TRUE : VK_FALSE};
	input = VkPartitionedAccelerationStructureInstancesInputNV{
	    .sType = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV,
	    .pNext = &flags,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .instanceCount = desc.InstanceCapacity,
	    .maxInstancePerPartitionCount = desc.MaxInstancesPerPartition,
	    .partitionCount = desc.PartitionCount,
	    .maxInstanceInGlobalPartitionCount = desc.MaxInstancesInGlobalPartition};
}

std::uint64_t VulkanPartitionedTlasServices::ResolveOperationArgumentGpuAddress(
    const RhiPartitionedTlasOperationHeader& operation,
    RhiGpuVirtualAddress instanceWriteAddress,
    RhiGpuVirtualAddress instanceUpdateAddress,
    RhiGpuVirtualAddress partitionTranslationAddress) noexcept
{
	if (operation.ArgumentData != 0)
	{
		return operation.ArgumentData;
	}
	switch (operation.Type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return instanceUpdateAddress;
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return partitionTranslationAddress;
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return instanceWriteAddress;
	}
}

std::uint64_t VulkanPartitionedTlasServices::ResolveOperationArgumentStride(
    const RhiPartitionedTlasOperationHeader& operation) noexcept
{
	if (operation.ArgumentStrideInBytes != 0)
	{
		return operation.ArgumentStrideInBytes;
	}
	switch (operation.Type)
	{
		case ERhiPartitionedTlasOperationType::UpdateInstance:
			return sizeof(VkPartitionedAccelerationStructureUpdateInstanceDataNV);
		case ERhiPartitionedTlasOperationType::WritePartitionTranslation:
			return sizeof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV);
		case ERhiPartitionedTlasOperationType::WriteInstance:
		default:
			return sizeof(VkPartitionedAccelerationStructureWriteInstanceDataNV);
	}
}

RhiGpuVirtualAddress VulkanPartitionedTlasServices::ResolvePartitionedInstanceAccelerationStructureAddress(
    RhiGpuVirtualAddress accelerationStructure) const noexcept
{
	if (m_memoryAllocator == nullptr || accelerationStructure == 0)
	{
		return accelerationStructure;
	}

	const VulkanGpuAllocationRecord* const record = m_memoryAllocator->FindAllocationRecordByDeviceAddress(accelerationStructure);
	if (record == nullptr || record->AccelerationStructure == VK_NULL_HANDLE || record->BufferDeviceAddress == 0)
	{
		return accelerationStructure;
	}

	return record->BufferDeviceAddress;
}

VulkanPartitionedTlasServices::VulkanPartitionedTlasServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_memoryAllocator(&memoryAllocator)
{
}

RhiPartitionedTlasBuildSizes VulkanPartitionedTlasServices::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || !capabilities.Groups.PartitionedTlas.Supported ||
	    m_rhi->GetPartitionedAccelerationStructureBuildSizes() == nullptr || desc.InstanceCapacity == 0 || desc.PartitionCount == 0)
	{
		return {};
	}

	VkPartitionedAccelerationStructureFlagsNV partitionedTlasFlags{};
	VkPartitionedAccelerationStructureInstancesInputNV input{};
	ConfigurePartitionedTlasInput(desc, input, partitionedTlasFlags);
	VkAccelerationStructureBuildSizesInfoKHR nativeInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
	    .pNext = nullptr};
	m_rhi->GetPartitionedAccelerationStructureBuildSizes()(m_rhi->GetDevice(), &input, &nativeInfo);
	return RhiPartitionedTlasBuildSizes{
	    .AccelerationStructureSizeInBytes = nativeInfo.accelerationStructureSize,
	    .BuildScratchSizeInBytes = nativeInfo.buildScratchSize,
	    .UpdateScratchSizeInBytes = nativeInfo.updateScratchSize,
	    .OperationInfoSizeInBytes =
	        sizeof(VkBuildPartitionedAccelerationStructureIndirectCommandNV) * static_cast<std::uint64_t>(desc.MaxOperations),
	    .OperationCountSizeInBytes = sizeof(std::uint32_t),
	    .InstanceWriteInfoSizeInBytes =
	        sizeof(VkPartitionedAccelerationStructureWriteInstanceDataNV) * static_cast<std::uint64_t>(desc.InstanceCapacity),
	    .InstanceUpdateInfoSizeInBytes =
	        desc.AllowInstanceUpdates ? sizeof(VkPartitionedAccelerationStructureUpdateInstanceDataNV) *
	                                        static_cast<std::uint64_t>(desc.InstanceCapacity)
	                                  : 0u,
	    .PartitionWriteInfoSizeInBytes =
	        desc.AllowPartitionTranslation ? sizeof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV) *
	                                             static_cast<std::uint64_t>(desc.PartitionCount + 1u)
	                                       : 0u};
}

RhiOwnedResourceHandle VulkanPartitionedTlasServices::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes& sizes,
    std::wstring_view debugName)
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !capabilities.Groups.PartitionedTlas.Supported ||
	    sizes.AccelerationStructureSizeInBytes == 0)
	{
		return {};
	}

	const RhiBufferResourceDesc desc{
	    .SizeInBytes = AlignUp(sizes.AccelerationStructureSizeInBytes, capabilities.AccelerationStructureByteAlignment),
	    .AllowUnorderedAccess = true};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RayTracingPartitionedTlasStorage" : debugName);
	if (record == nullptr || record->DeviceAddress == 0)
	{
		return {};
	}

	record->AccelerationStructureType = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
	record->IsPartitionedAccelerationStructure = true;
	SetVulkanAllocationRecordDebugName(*record, debugName.empty() ? L"RayTracingPartitionedTlasStorage" : debugName);
	return MakeVulkanOwnedResourceHandle(std::move(record));
}

RhiOwnedResourceHandle VulkanPartitionedTlasServices::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc& operationPack,
    std::wstring_view debugName)
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !capabilities.Groups.PartitionedTlas.Supported ||
	    operationPack.OperationCount == 0 || operationPack.Operations == nullptr)
	{
		return {};
	}
	if ((operationPack.InstanceWriteCount > 0 && operationPack.InstanceWrites == nullptr) ||
	    (operationPack.InstanceUpdateCount > 0 && operationPack.InstanceUpdates == nullptr) ||
	    (operationPack.PartitionTranslationCount > 0 && operationPack.PartitionTranslations == nullptr))
	{
		return {};
	}

	const std::uint64_t countOffset = 0;
	const std::uint64_t operationOffset = AlignUp(countOffset + sizeof(std::uint32_t), alignof(VkBuildPartitionedAccelerationStructureIndirectCommandNV));
	const std::uint64_t operationBytes =
	    sizeof(VkBuildPartitionedAccelerationStructureIndirectCommandNV) * static_cast<std::uint64_t>(operationPack.OperationCount);
	const std::uint64_t instanceWriteOffset =
	    AlignUp(operationOffset + operationBytes, alignof(VkPartitionedAccelerationStructureWriteInstanceDataNV));
	const std::uint64_t instanceWriteBytes =
	    sizeof(VkPartitionedAccelerationStructureWriteInstanceDataNV) * static_cast<std::uint64_t>(operationPack.InstanceWriteCount);
	const std::uint64_t instanceUpdateOffset =
	    AlignUp(instanceWriteOffset + instanceWriteBytes, alignof(VkPartitionedAccelerationStructureUpdateInstanceDataNV));
	const std::uint64_t instanceUpdateBytes =
	    sizeof(VkPartitionedAccelerationStructureUpdateInstanceDataNV) * static_cast<std::uint64_t>(operationPack.InstanceUpdateCount);
	const std::uint64_t partitionTranslationOffset =
	    AlignUp(instanceUpdateOffset + instanceUpdateBytes, alignof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV));
	const std::uint64_t partitionTranslationBytes = sizeof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV) *
	                                               static_cast<std::uint64_t>(operationPack.PartitionTranslationCount);
	const std::uint64_t totalSize = AlignUp(partitionTranslationOffset + partitionTranslationBytes, 16);
	if (totalSize == 0)
	{
		return {};
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = totalSize};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"RayTracingPartitionedTlasOperations" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || record->DeviceAddress == 0)
	{
		return {};
	}

	std::vector<std::uint8_t> packed(static_cast<std::size_t>(totalSize), 0);
	std::memcpy(packed.data() + countOffset, &operationPack.OperationCount, sizeof(operationPack.OperationCount));

	auto* const nativeOperations = reinterpret_cast<VkBuildPartitionedAccelerationStructureIndirectCommandNV*>(
	    packed.data() + static_cast<std::size_t>(operationOffset));
	auto* const nativeInstanceWrites = reinterpret_cast<VkPartitionedAccelerationStructureWriteInstanceDataNV*>(
	    packed.data() + static_cast<std::size_t>(instanceWriteOffset));
	auto* const nativeInstanceUpdates = reinterpret_cast<VkPartitionedAccelerationStructureUpdateInstanceDataNV*>(
	    packed.data() + static_cast<std::size_t>(instanceUpdateOffset));
	auto* const nativePartitionTranslations =
	    reinterpret_cast<VkPartitionedAccelerationStructureWritePartitionTranslationDataNV*>(
	        packed.data() + static_cast<std::size_t>(partitionTranslationOffset));

	const RhiGpuVirtualAddress instanceWriteAddress = record->DeviceAddress + instanceWriteOffset;
	const RhiGpuVirtualAddress instanceUpdateAddress = record->DeviceAddress + instanceUpdateOffset;
	const RhiGpuVirtualAddress partitionTranslationAddress = record->DeviceAddress + partitionTranslationOffset;
	for (std::uint32_t operationIndex = 0; operationIndex < operationPack.OperationCount; ++operationIndex)
	{
		const RhiPartitionedTlasOperationHeader& operation = operationPack.Operations[operationIndex];
		nativeOperations[operationIndex] = VkBuildPartitionedAccelerationStructureIndirectCommandNV{
		    .opType = ToVkPartitionedOperationType(operation.Type),
		    .argCount = operation.ArgumentCount,
		    .argData =
		        VkStridedDeviceAddressNV{
		            .startAddress = ResolveOperationArgumentGpuAddress(
		                operation,
		                instanceWriteAddress,
		                instanceUpdateAddress,
		                partitionTranslationAddress),
		            .strideInBytes = ResolveOperationArgumentStride(operation)}};
	}

	for (std::uint32_t instanceIndex = 0; instanceIndex < operationPack.InstanceWriteCount; ++instanceIndex)
	{
		const RhiPartitionedTlasInstanceWriteDesc& source = operationPack.InstanceWrites[instanceIndex];
		VkPartitionedAccelerationStructureWriteInstanceDataNV& target = nativeInstanceWrites[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < source.Transform.size(); ++transformIndex)
		{
			target.transform.matrix[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		std::memcpy(target.explicitAABB, source.ExplicitBoundingBox.data(), sizeof(target.explicitAABB));
		target.instanceID = source.InstanceID;
		target.instanceMask = source.InstanceMask;
		target.instanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex;
		target.instanceFlags = ToVkPartitionedInstanceFlags(source.Flags);
		target.instanceIndex = source.InstanceIndex;
		target.partitionIndex = source.PartitionIndex;
		target.accelerationStructure = ResolvePartitionedInstanceAccelerationStructureAddress(source.AccelerationStructure);
	}

	for (std::uint32_t instanceIndex = 0; instanceIndex < operationPack.InstanceUpdateCount; ++instanceIndex)
	{
		const RhiPartitionedTlasInstanceUpdateDesc& source = operationPack.InstanceUpdates[instanceIndex];
		nativeInstanceUpdates[instanceIndex] = VkPartitionedAccelerationStructureUpdateInstanceDataNV{
		    .instanceIndex = source.InstanceIndex,
		    .instanceContributionToHitGroupIndex = source.InstanceContributionToHitGroupIndex,
		    .accelerationStructure = ResolvePartitionedInstanceAccelerationStructureAddress(source.AccelerationStructure)};
	}

	for (std::uint32_t partitionIndex = 0; partitionIndex < operationPack.PartitionTranslationCount; ++partitionIndex)
	{
		const RhiPartitionedTlasPartitionTranslationDesc& source = operationPack.PartitionTranslations[partitionIndex];
		VkPartitionedAccelerationStructureWritePartitionTranslationDataNV& target = nativePartitionTranslations[partitionIndex];
		target.partitionIndex = source.PartitionIndex;
		target.partitionTranslation[0] = source.Translation[0];
		target.partitionTranslation[1] = source.Translation[1];
		target.partitionTranslation[2] = source.Translation[2];
	}

	if (!m_memoryAllocator->WriteAllocation(*record, packed.data(), packed.size()))
	{
		return {};
	}
	return MakeVulkanOwnedResourceHandle(std::move(record));
}

RhiPartitionedTlasOperationBufferLayout
VulkanPartitionedTlasServices::GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	const RhiRayTracingCapabilities capabilities = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
	if (m_rhi == nullptr || !capabilities.Groups.PartitionedTlas.Supported || desc.MaxOperations == 0)
	{
		return {};
	}

	RhiPartitionedTlasOperationBufferLayout layout{};
	layout.OperationCountOffsetInBytes = 0;
	layout.OperationHeaderStrideInBytes = sizeof(VkBuildPartitionedAccelerationStructureIndirectCommandNV);
	layout.InstanceWriteStrideInBytes = sizeof(VkPartitionedAccelerationStructureWriteInstanceDataNV);
	layout.InstanceUpdateStrideInBytes = sizeof(VkPartitionedAccelerationStructureUpdateInstanceDataNV);
	layout.PartitionTranslationStrideInBytes = sizeof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV);
	layout.OperationHeadersOffsetInBytes = AlignUp(sizeof(std::uint32_t), alignof(VkBuildPartitionedAccelerationStructureIndirectCommandNV));
	layout.InstanceWriteRecordsOffsetInBytes =
	    AlignUp(layout.OperationHeadersOffsetInBytes + layout.OperationHeaderStrideInBytes * desc.MaxOperations,
	        alignof(VkPartitionedAccelerationStructureWriteInstanceDataNV));
	std::uint64_t cursor = layout.InstanceWriteRecordsOffsetInBytes + layout.InstanceWriteStrideInBytes * desc.InstanceCapacity;
	if (desc.AllowInstanceUpdates)
	{
		layout.InstanceUpdateRecordsOffsetInBytes =
		    AlignUp(cursor, alignof(VkPartitionedAccelerationStructureUpdateInstanceDataNV));
		cursor = layout.InstanceUpdateRecordsOffsetInBytes + layout.InstanceUpdateStrideInBytes * desc.InstanceCapacity;
	}
	else
	{
		layout.InstanceUpdateRecordsOffsetInBytes = cursor;
	}
	if (desc.AllowPartitionTranslation)
	{
		layout.PartitionTranslationRecordsOffsetInBytes =
		    AlignUp(cursor, alignof(VkPartitionedAccelerationStructureWritePartitionTranslationDataNV));
		cursor = layout.PartitionTranslationRecordsOffsetInBytes +
		         layout.PartitionTranslationStrideInBytes * static_cast<std::uint64_t>(desc.PartitionCount + 1u);
	}
	else
	{
		layout.PartitionTranslationRecordsOffsetInBytes = cursor;
	}
	layout.TotalSizeInBytes = AlignUp(cursor, 16);
	return layout;
}
