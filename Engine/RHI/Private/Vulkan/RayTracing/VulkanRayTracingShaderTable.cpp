#include "Vulkan/VulkanPCH.h"

#include "Vulkan/RayTracing/VulkanRayTracingShaderTable.h"

#include "Core/Public/Diagnostics/Error.h"
#include "RayTracing/RhiRayTracingShaderTablePacking.h"
#include "Validation/RhiContract.h"
#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanRayTracingPipeline.h"

#include <cstring>
#include <limits>
#include <vector>

std::vector<std::byte> VulkanRayTracingShaderTable::CollectShaderIdentifiers(
    const VulkanRayTracingPipeline& pipeline,
    std::span<const RhiRayTracingShaderRecord> records,
    std::span<const std::byte> groupHandles,
    std::uint32_t handleSize)
{
	if (handleSize == 0 || records.size() > std::numeric_limits<std::size_t>::max() / handleSize)
	{
		throw Diagnostics::Error("Vulkan shader-table identifier storage overflowed.");
	}
	std::vector<std::byte> identifiers(records.size() * handleSize);
	for (std::size_t index = 0; index < records.size(); ++index)
	{
		const RhiRayTracingShaderRecord& record = records[index];
		const std::uint32_t groupIndex = pipeline.FindShaderGroup(record.ExportName);
		if (groupIndex == std::numeric_limits<std::uint32_t>::max()
		    || groupIndex > std::numeric_limits<std::uint64_t>::max() / handleSize)
		{
			throw Diagnostics::Error("Vulkan shader-table record references an export absent from its pipeline generation.");
		}
		const std::uint64_t handleOffset = static_cast<std::uint64_t>(groupIndex) * handleSize;
		if (handleOffset > groupHandles.size() || handleSize > groupHandles.size() - handleOffset)
		{
			throw Diagnostics::Error("Vulkan shader-table record references an export absent from its pipeline generation.");
		}
		std::byte* const destination = identifiers.data() + index * handleSize;
		std::memcpy(destination, groupHandles.data() + handleOffset, handleSize);
	}
	return identifiers;
}

VulkanRayTracingShaderTable::VulkanRayTracingShaderTable(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    const RayTracingShaderTableDesc& desc) :
    RayTracingShaderTable(desc.Generation, desc.Pipeline != nullptr ? desc.Pipeline->GetGeneration() : 0)
{
	RhiContract::ValidateRayTracingShaderTableDesc(desc);
	const auto* pipeline = dynamic_cast<const VulkanRayTracingPipeline*>(desc.Pipeline);
	const RhiRayTracingCapabilities capabilities = rhi.GetRayTracingCapabilities();
	if (pipeline == nullptr || !capabilities.SupportsRayTracingPipeline || rhi.GetRayTracingShaderGroupHandles() == nullptr)
	{
		throw Diagnostics::Error("Vulkan shader-table creation requires a ready Vulkan pipeline.");
	}
	const std::uint64_t handleBytes =
	    static_cast<std::uint64_t>(pipeline->GetShaderGroupCount()) * capabilities.ShaderGroupHandleSizeInBytes;
	if (handleBytes == 0 || handleBytes > std::numeric_limits<std::size_t>::max())
	{
		throw Diagnostics::Error("Vulkan shader-group handle size is invalid.");
	}
	std::vector<std::byte> groupHandles(static_cast<std::size_t>(handleBytes));
	const VkResult handleResult = rhi.GetRayTracingShaderGroupHandles()(
	    rhi.GetDevice(),
	    pipeline->GetPipeline(),
	    0,
	    pipeline->GetShaderGroupCount(),
	    groupHandles.size(),
	    groupHandles.data());
	if (!VulkanResult::Succeeded(handleResult))
	{
		throw Diagnostics::Error(VulkanResult::FormatFailure("vkGetRayTracingShaderGroupHandlesKHR", handleResult));
	}
	const RhiRayTracingShaderTablePackingRules packingRules{
	    .IdentifierSizeInBytes = capabilities.ShaderGroupHandleSizeInBytes,
	    .RecordAlignmentInBytes = capabilities.ShaderTableRecordAlignmentInBytes,
	    .TableAlignmentInBytes = capabilities.ShaderTableAlignmentInBytes,
	    .MaximumRecordStrideInBytes = capabilities.MaxShaderTableRecordStrideInBytes};
	std::vector<std::byte> bytes;
	m_rayGeneration = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.RayGenerationRecords,
	    CollectShaderIdentifiers(
	        *pipeline,
	        desc.RayGenerationRecords,
	        groupHandles,
	        capabilities.ShaderGroupHandleSizeInBytes),
	    packingRules,
	    bytes);
	m_miss = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.MissRecords,
	    CollectShaderIdentifiers(
	        *pipeline,
	        desc.MissRecords,
	        groupHandles,
	        capabilities.ShaderGroupHandleSizeInBytes),
	    packingRules,
	    bytes);
	m_hitGroup = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.HitGroupRecords,
	    CollectShaderIdentifiers(
	        *pipeline,
	        desc.HitGroupRecords,
	        groupHandles,
	        capabilities.ShaderGroupHandleSizeInBytes),
	    packingRules,
	    bytes);
	m_callable = RhiRayTracingShaderTablePacking::AppendRegion(
	    desc.CallableRecords,
	    CollectShaderIdentifiers(
	        *pipeline,
	        desc.CallableRecords,
	        groupHandles,
	        capabilities.ShaderGroupHandleSizeInBytes),
	    packingRules,
	    bytes);

	VkBufferCreateInfo bufferInfo{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .size = bytes.size(),
	    .usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
	    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	    .queueFamilyIndexCount = 0,
	    .pQueueFamilyIndices = nullptr};
	rhi.ConfigureResourceQueueSharing(bufferInfo);
	m_allocation = memoryAllocator.CreateBuffer(
	    bufferInfo,
	    RhiMemoryCategory::RayTracing,
	    RhiMemoryResidencyClass::HostUpload,
	    desc.DebugName != nullptr ? std::wstring_view(desc.DebugName) : std::wstring_view(L"RHI_RayTracingShaderTable"));
	if (m_allocation == nullptr || m_allocation->Buffer == VK_NULL_HANDLE || m_allocation->BufferDeviceAddress == 0
	    || !memoryAllocator.WriteAllocation(*m_allocation, bytes.data(), bytes.size()))
	{
		throw Diagnostics::Error("Vulkan shader-table allocation or upload failed.");
	}
}
