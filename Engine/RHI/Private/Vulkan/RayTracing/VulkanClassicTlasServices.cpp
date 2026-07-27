#include "Vulkan/VulkanPCH.h"

#include "Vulkan/RayTracing/VulkanClassicTlasServices.h"

#include "Memory/RhiMemoryTypes.h"
#include "Resources/RhiResourceDesc.h"
#include "Validation/RhiContract.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <memory>
#include <vector>

class VulkanTlasInstanceTranslation final
{
  public:
	static VkGeometryInstanceFlagsKHR ToNativeInstanceFlags(RhiRayTracingInstanceFlags flags) noexcept
	{
		VkGeometryInstanceFlagsKHR nativeFlags = 0;
		if (HasFlag(flags, RhiRayTracingInstanceFlags::TriangleFacingCullDisable))
		{
			nativeFlags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
		}
		if (HasFlag(flags, RhiRayTracingInstanceFlags::ForceOpaque))
		{
			nativeFlags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
		}
		if (HasFlag(flags, RhiRayTracingInstanceFlags::ForceNonOpaque))
		{
			nativeFlags |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
		}
		return nativeFlags;
	}
};

VulkanClassicTlasServices::VulkanClassicTlasServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi), m_memoryAllocator(&memoryAllocator)
{
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanClassicTlasServices::GetClassicTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetAccelerationStructureBuildSizes() == nullptr || instanceCount == 0)
	{
		return {};
	}

	const VkAccelerationStructureGeometryInstancesDataKHR instances{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
	    .pNext = nullptr,
	    .arrayOfPointers = VK_FALSE,
	    .data = VkDeviceOrHostAddressConstKHR{.deviceAddress = 0}};
	const VkAccelerationStructureGeometryKHR geometry{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = nullptr,
	    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
	    .geometry = VkAccelerationStructureGeometryDataKHR{.instances = instances},
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR};
	const VkBuildAccelerationStructureFlagsKHR nativeBuildFlags =
	    VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
	    (HasFlag(buildFlags, ERhiClassicTlasBuildFlags::AllowUpdate)
	         ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
	         : static_cast<VkBuildAccelerationStructureFlagsKHR>(0));
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .flags = nativeBuildFlags,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = VK_NULL_HANDLE,
	    .geometryCount = 1,
	    .pGeometries = &geometry,
	    .ppGeometries = nullptr,
	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = 0}};
	VkAccelerationStructureBuildSizesInfoKHR nativeInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	m_rhi->GetAccelerationStructureBuildSizes()(
	    m_rhi->GetDevice(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &buildInfo,
	    &instanceCount,
	    &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.accelerationStructureSize,
	    .ScratchDataSizeInBytes = nativeInfo.buildScratchSize,
	    .UpdateScratchDataSizeInBytes = nativeInfo.updateScratchSize};
}

RhiOwnedResourceHandle VulkanClassicTlasServices::CreateClassicTopLevelAccelerationStructureInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    !RhiContract::IsRayTracingInstanceListUsable(instances, instanceCount))
	{
		return {};
	}

	std::vector<VkAccelerationStructureInstanceKHR> nativeInstances(instanceCount);
	for (std::uint32_t instanceIndex = 0; instanceIndex < instanceCount; ++instanceIndex)
	{
		const RhiRayTracingInstanceDesc& source = instances[instanceIndex];
		VkAccelerationStructureInstanceKHR& nativeInstance = nativeInstances[instanceIndex];
		for (std::uint32_t transformIndex = 0; transformIndex < source.Transform.size(); ++transformIndex)
		{
			nativeInstance.transform.matrix[transformIndex / 4][transformIndex % 4] = source.Transform[transformIndex];
		}
		nativeInstance.instanceCustomIndex = source.InstanceID & 0x00FFFFFFu;
		nativeInstance.mask = source.InstanceMask & 0xFFu;
		nativeInstance.instanceShaderBindingTableRecordOffset = source.InstanceContributionToHitGroupIndex & 0x00FFFFFFu;
		nativeInstance.flags = VulkanTlasInstanceTranslation::ToNativeInstanceFlags(source.Flags);
		nativeInstance.accelerationStructureReference = source.AccelerationStructure;
	}

	const std::uint64_t sizeInBytes = sizeof(VkAccelerationStructureInstanceKHR) * static_cast<std::uint64_t>(nativeInstances.size());
	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"RayTracingClassicTlasInstanceBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE ||
	    !m_memoryAllocator->WriteAllocation(*record, nativeInstances.data(), static_cast<std::size_t>(sizeInBytes)))
	{
		return {};
	}
	return MakeVulkanOwnedResourceHandle(std::move(record));
}
