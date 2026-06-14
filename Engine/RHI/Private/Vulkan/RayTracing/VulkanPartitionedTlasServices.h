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
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureLogicalUpdateBuffer(
	    const RhiPartitionedTlasLogicalUpdateBufferDesc& desc,
	    const RhiPartitionedTlasLogicalUpdateRecord* records,
	    std::uint32_t recordCount,
	    std::wstring_view debugName) override;
	RhiPartitionedTlasGpuOperationBufferLayout GetPartitionedTopLevelAccelerationStructureGpuOperationBufferLayout(
	    const RhiPartitionedTlasDesc& desc) const noexcept override;
	RhiOwnedResourceHandle CreatePartitionedTopLevelAccelerationStructureGpuOperationBuffer(
	    const RhiPartitionedTlasGpuOperationBufferDesc& desc,
	    std::wstring_view debugName) override;
	bool PackPartitionedTopLevelAccelerationStructureGpuOperations(
	    RenderCommandList& commandList,
	    const RhiPartitionedTlasGpuOperationPackDesc& desc) noexcept override;

  private:
	static std::uint64_t AlignUp(std::uint64_t value, std::uint64_t alignment) noexcept;
	static VkPartitionedAccelerationStructureInstanceFlagsNV ToVkPartitionedInstanceFlags(
	    RhiPartitionedTlasInstanceFlags flags) noexcept;
	static VkPartitionedAccelerationStructureOpTypeNV ToVkPartitionedOperationType(ERhiPartitionedTlasOperationType type) noexcept;
	static VkPartitionedAccelerationStructureInstancesInputNV BuildPartitionedTlasInput(const RhiPartitionedTlasDesc& desc) noexcept;
	static std::uint64_t ResolveOperationArgumentGpuAddress(
	    const RhiPartitionedTlasOperationHeader& operation,
	    RhiGpuVirtualAddress instanceWriteAddress,
	    RhiGpuVirtualAddress instanceUpdateAddress,
	    RhiGpuVirtualAddress partitionTranslationAddress) noexcept;
	static std::uint64_t ResolveOperationArgumentStride(const RhiPartitionedTlasOperationHeader& operation) noexcept;

	VulkanRhi* m_rhi = nullptr;
	VulkanGpuMemoryAllocator* m_memoryAllocator = nullptr;
};
