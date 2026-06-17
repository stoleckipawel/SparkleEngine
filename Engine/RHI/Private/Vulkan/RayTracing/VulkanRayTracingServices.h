#pragma once

#include "RayTracing/RhiRayTracingService.h"
#include "Vulkan/RayTracing/VulkanClassicTlasServices.h"
#include "Vulkan/RayTracing/VulkanPartitionedTlasServices.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <string_view>

class VulkanGpuMemoryAllocator;
class VulkanRhi;

class VulkanRayTracingServices final : public RhiRayTracingService
{
  public:
	VulkanRayTracingServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept;

	RhiClassicTlasService& GetClassicTlasService() noexcept override;
	const RhiClassicTlasService& GetClassicTlasService() const noexcept override;
	RhiPartitionedTlasService& GetPartitionedTlasService() noexcept override;
	const RhiPartitionedTlasService& GetPartitionedTlasService() const noexcept override;
	RhiRayTracingCapabilities GetCapabilities() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetBottomLevelAccelerationStructurePrebuildInfo(
	    const RhiRayTracingGeometryDesc& geometry) const noexcept override;
	RhiRayTracingAccelerationStructurePrebuildInfo GetTopLevelAccelerationStructurePrebuildInfo(
	    std::uint32_t instanceCount,
	    ERhiClassicTlasBuildFlags buildFlags = ERhiClassicTlasBuildFlags::None) const noexcept override;
	RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingScratchBuffer(std::uint64_t sizeInBytes, std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingAccelerationStructureBuffer(
	    std::uint64_t sizeInBytes,
	    ERhiRayTracingAccelerationStructureType type,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreateInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName);
	RhiOwnedResourceHandle CreateRayTracingInstanceBuffer(
	    const RhiRayTracingInstanceDesc* instances,
	    std::uint32_t instanceCount,
	    std::wstring_view debugName) override;

  private:
	static VkAccelerationStructureTypeKHR ToVkAccelerationStructureType(ERhiRayTracingAccelerationStructureType type) noexcept;
	static VkAccelerationStructureGeometryKHR BuildBottomLevelGeometry(const RhiRayTracingGeometryDesc& geometry) noexcept;

	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
	VulkanClassicTlasServices m_classicTlasServices;
	VulkanPartitionedTlasServices m_partitionedTlasServices;
};
