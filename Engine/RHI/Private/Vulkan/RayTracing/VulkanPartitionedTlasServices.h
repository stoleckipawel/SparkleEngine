#pragma once

#include "Interop/RhiNativeHandles.h"
#include "RayTracing/RhiPartitionedTlasService.h"
#include "Vulkan/VulkanIncludes.h"

#include <cstdint>
#include <string_view>

class VulkanGpuMemoryAllocator;
class VulkanRhi;

class VulkanPartitionedTlasServices final : public RhiPartitionedTlasService
{
  public:
	VulkanPartitionedTlasServices(VulkanRhi& rhi, VulkanGpuMemoryAllocator& memoryAllocator) noexcept;

	RhiPartitionedTlasBuildSizes GetPartitionedTopLevelAccelerationStructureBuildSizes(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureBuffer(
	    const RhiPartitionedTlasBuildSizes& sizes,
	    std::wstring_view debugName) override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureOperationBuffer(
	    const RhiPartitionedTlasOperationPackDesc& operationPack,
	    std::wstring_view debugName) override;
	RhiPartitionedTlasOperationBufferLayout GetPartitionedTopLevelAccelerationStructureOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;

  private:
	static std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;
	static VkPartitionedAccelerationStructureInstanceFlagsNV ToVkPartitionedInstanceFlags(
	    RhiPartitionedTlasInstanceFlags flags) noexcept;
	static VkPartitionedAccelerationStructureOpTypeNV ToVkPartitionedOperationType(ERhiPartitionedTlasOperationType type) noexcept;
	static void ConfigurePartitionedTlasInput(
	    const RhiPartitionedTlasDesc& desc,
	    VkPartitionedAccelerationStructureInstancesInputNV& input,
	    VkPartitionedAccelerationStructureFlagsNV& flags) noexcept;
	static std::uint64_t ResolveOperationArgumentGpuAddress(
	    const RhiPartitionedTlasOperationHeader& operation,
	    RhiGpuVirtualAddress instanceWriteAddress,
	    RhiGpuVirtualAddress instanceUpdateAddress,
	    RhiGpuVirtualAddress partitionTranslationAddress) noexcept;
	static std::uint64_t ResolveOperationArgumentStride(const RhiPartitionedTlasOperationHeader& operation) noexcept;
	RhiGpuVirtualAddress ResolvePartitionedInstanceAccelerationStructureAddress(RhiGpuVirtualAddress accelerationStructure) const noexcept;

	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
};
