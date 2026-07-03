#include "Vulkan/VulkanPCH.h"

#include "Vulkan/RayTracing/VulkanRayTracingServices.h"

#include "Validation/RhiContract.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <memory>

VkAccelerationStructureTypeKHR VulkanRayTracingServices::ToVkAccelerationStructureType(
    ERhiRayTracingAccelerationStructureType type) noexcept
{
	switch (type)
	{
		case ERhiRayTracingAccelerationStructureType::TopLevel:
			return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		case ERhiRayTracingAccelerationStructureType::BottomLevel:
		default:
			return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
	}
}

VkAccelerationStructureGeometryKHR VulkanRayTracingServices::BuildBottomLevelGeometry(
    const RhiRayTracingGeometryDesc& geometry) noexcept
{
	const VkAccelerationStructureGeometryTrianglesDataKHR triangles{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
	    .pNext = nullptr,
	    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
	    .vertexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.VertexBuffer},
	    .vertexStride = geometry.VertexStrideInBytes,
	    .maxVertex = geometry.VertexCount > 0 ? geometry.VertexCount - 1u : 0u,
	    .indexType = VulkanTypeConversions::ToVkIndexType(geometry.IndexFormat),
	    .indexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.IndexBuffer},
	    .transformData = VkDeviceOrHostAddressConstKHR{.deviceAddress = 0}};
	return VkAccelerationStructureGeometryKHR{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = nullptr,
	    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
	    .geometry = VkAccelerationStructureGeometryDataKHR{.triangles = triangles},
	    .flags = geometry.Opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0u};
}

VulkanRayTracingServices::VulkanRayTracingServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept :
    m_rhi(&rhi),
    m_memoryAllocator(&memoryAllocator),
    m_classicTlasServices(rhi, memoryAllocator),
    m_partitionedTlasServices(rhi, memoryAllocator)
{
}

RhiClassicTlasService& VulkanRayTracingServices::GetClassicTlasService() noexcept
{
	return m_classicTlasServices;
}

const RhiClassicTlasService& VulkanRayTracingServices::GetClassicTlasService() const noexcept
{
	return m_classicTlasServices;
}

RhiPartitionedTlasService& VulkanRayTracingServices::GetPartitionedTlasService() noexcept
{
	return m_partitionedTlasServices;
}

const RhiPartitionedTlasService& VulkanRayTracingServices::GetPartitionedTlasService() const noexcept
{
	return m_partitionedTlasServices;
}

RhiRayTracingCapabilities VulkanRayTracingServices::GetCapabilities() const noexcept
{
	return m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities() : RhiRayTracingCapabilities{};
}

RhiRayTracingCapabilities VulkanRayTracingServices::GetRayTracingCapabilities() const noexcept
{
	return GetCapabilities();
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRayTracingServices::GetBottomLevelAccelerationStructurePrebuildInfo(
    const RhiRayTracingGeometryDesc& geometry) const noexcept
{
	if (m_rhi == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetAccelerationStructureBuildSizes() == nullptr ||
	    !RhiContract::IsRayTracingGeometryDescUsable(geometry))
	{
		return {};
	}

	const VkAccelerationStructureGeometryKHR nativeGeometry = BuildBottomLevelGeometry(geometry);
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = VK_NULL_HANDLE,
	    .geometryCount = 1,
	    .pGeometries = &nativeGeometry,
	    .ppGeometries = nullptr,
	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = 0}};
	const std::uint32_t primitiveCount = geometry.IndexCount / 3u;
	VkAccelerationStructureBuildSizesInfoKHR nativeInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
	m_rhi->GetAccelerationStructureBuildSizes()(
	    m_rhi->GetDevice(),
	    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
	    &buildInfo,
	    &primitiveCount,
	    &nativeInfo);
	return RhiRayTracingAccelerationStructurePrebuildInfo{
	    .ResultDataMaxSizeInBytes = nativeInfo.accelerationStructureSize,
	    .ScratchDataSizeInBytes = nativeInfo.buildScratchSize,
	    .UpdateScratchDataSizeInBytes = nativeInfo.updateScratchSize};
}

RhiRayTracingAccelerationStructurePrebuildInfo VulkanRayTracingServices::GetTopLevelAccelerationStructurePrebuildInfo(
    std::uint32_t instanceCount,
    ERhiClassicTlasBuildFlags buildFlags) const noexcept
{
	return m_classicTlasServices.GetClassicTopLevelAccelerationStructurePrebuildInfo(instanceCount, buildFlags);
}

RhiPartitionedTlasBuildSizes VulkanRayTracingServices::GetPartitionedTopLevelAccelerationStructureBuildSizes(
    const RhiPartitionedTlasDesc& desc) const noexcept
{
	return m_partitionedTlasServices.GetPartitionedTopLevelAccelerationStructureBuildSizes(desc);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreatePartitionedTopLevelAccelerationStructureBuffer(
    const RhiPartitionedTlasBuildSizes& sizes,
    std::wstring_view debugName)
{
	return m_partitionedTlasServices.CreatePartitionedTopLevelAccelerationStructureBuffer(sizes, debugName);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
    const RhiPartitionedTlasOperationPackDesc& operationPack,
    std::wstring_view debugName)
{
	return m_partitionedTlasServices.CreatePartitionedTopLevelAccelerationStructureOperationBuffer(operationPack, debugName);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName)
{
	const std::uint64_t scratchAlignment = m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().ScratchBufferByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    !RhiContract::IsRayTracingBufferSizeUsable(sizeInBytes, scratchAlignment))
	{
		return {};
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true};
	const VkBufferCreateInfo bufferCreateInfo =
	    VulkanTypeConversions::BuildBufferCreateInfo(desc, VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RayTracingScratch" : debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateRayTracingScratchBuffer(
    std::uint64_t sizeInBytes,
    std::wstring_view debugName)
{
	return CreateScratchBuffer(sizeInBytes, debugName);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	const std::uint64_t asAlignment =
	    m_rhi != nullptr ? m_rhi->GetRayTracingCapabilities().AccelerationStructureByteAlignment : 0;
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || !m_rhi->GetRayTracingCapabilities().SupportsRayTracing ||
	    m_rhi->GetCreateAccelerationStructure() == nullptr || m_rhi->GetAccelerationStructureDeviceAddress() == nullptr ||
	    !RhiContract::IsRayTracingBufferSizeUsable(sizeInBytes, asAlignment))
	{
		return {};
	}

	const VkAccelerationStructureTypeKHR nativeType = ToVkAccelerationStructureType(type);
	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .AllowUnorderedAccess = true};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::DeviceLocal,
	    debugName.empty() ? L"RayTracingAccelerationStructure" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE)
	{
		return {};
	}

	const VkAccelerationStructureCreateInfoKHR createInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
	    .pNext = nullptr,
	    .createFlags = 0,
	    .buffer = record->Buffer,
	    .offset = 0,
	    .size = sizeInBytes,
	    .type = nativeType,
	    .deviceAddress = 0};
	VkAccelerationStructureKHR accelerationStructure = VK_NULL_HANDLE;
	const VkResult result =
	    m_rhi->GetCreateAccelerationStructure()(m_rhi->GetDevice(), &createInfo, nullptr, &accelerationStructure);
	if (!VulkanResult::Succeeded(result) || accelerationStructure == VK_NULL_HANDLE)
	{
		return {};
	}

	record->AccelerationStructure = accelerationStructure;
	record->AccelerationStructureType = nativeType;
	const VkAccelerationStructureDeviceAddressInfoKHR addressInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
	    .pNext = nullptr,
	    .accelerationStructure = accelerationStructure};
	record->DeviceAddress = m_rhi->GetAccelerationStructureDeviceAddress()(m_rhi->GetDevice(), &addressInfo);
	SetVulkanAllocationRecordDebugName(*record, debugName.empty() ? L"RayTracingAccelerationStructure" : debugName);
	return record->DeviceAddress != 0 ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateRayTracingAccelerationStructureBuffer(
    std::uint64_t sizeInBytes,
    ERhiRayTracingAccelerationStructureType type,
    std::wstring_view debugName)
{
	return CreateAccelerationStructureBuffer(sizeInBytes, type, debugName);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return m_classicTlasServices.CreateClassicTopLevelAccelerationStructureInstanceBuffer(instances, instanceCount, debugName);
}

RhiOwnedResourceHandle VulkanRayTracingServices::CreateRayTracingInstanceBuffer(
    const RhiRayTracingInstanceDesc* instances,
    std::uint32_t instanceCount,
    std::wstring_view debugName)
{
	return CreateInstanceBuffer(instances, instanceCount, debugName);
}
