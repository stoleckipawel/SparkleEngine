#include "Vulkan/Resources/VulkanResourceService.h"

#include "Resources/Texture.h"
#include "Validation/RhiContract.h"
#include "Vulkan/Commands/VulkanCommandContext.h"
#include "Vulkan/Descriptors/VulkanDescriptorManager.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Resources/VulkanTexture.h"
#include "Vulkan/Textures/VulkanTextureFactory.h"
#include "Vulkan/VulkanTypeConversions.h"

VulkanResourceService::VulkanResourceService(
    VulkanRhi& rhi,
    VulkanCommandContext& commandContext,
    VulkanGpuMemoryAllocator& memoryAllocator,
    VulkanDescriptorManager& descriptorManager,
    const RhiCapabilities& capabilities) noexcept :
    m_rhi(&rhi),
    m_commandContext(&commandContext),
    m_memoryAllocator(&memoryAllocator),
    m_descriptorManager(&descriptorManager),
    m_capabilities(&capabilities),
    m_textureFactory(std::make_unique<VulkanTextureFactory>(memoryAllocator))
{
}

VulkanResourceService::~VulkanResourceService() noexcept
{
	m_textureFactory.reset();
	FlushDeferredResourceReleases();
}

std::unique_ptr<Texture> VulkanResourceService::CreateTexture(RhiTextureUploadDesc textureUpload, std::wstring_view debugName)
{
	if (m_rhi == nullptr || m_memoryAllocator == nullptr || m_descriptorManager == nullptr || !textureUpload.IsValid())
	{
		return {};
	}

	return std::make_unique<VulkanTexture>(
	    *m_rhi,
	    *m_memoryAllocator,
	    *m_descriptorManager,
	    std::move(textureUpload),
	    debugName.empty() ? L"VulkanTexture" : debugName);
}

RhiOwnedResourceHandle VulkanResourceService::CreateTextureResource(
    const RhiTextureResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	(void) initialState;
	if (m_textureFactory == nullptr || m_capabilities == nullptr || !RhiContract::IsTextureResourceDescUsable(*m_capabilities, desc))
	{
		return {};
	}

	return m_textureFactory->CreateTextureResource(desc, category, residencyClass, debugName);
}

RhiOwnedResourceHandle VulkanResourceService::CreateBufferResource(
    const RhiBufferResourceDesc& desc,
    ResourceState initialState,
    RhiMemoryCategory category,
    RhiMemoryResidencyClass residencyClass,
    std::wstring_view debugName)
{
	(void) initialState;
	if (m_memoryAllocator == nullptr || desc.SizeInBytes == 0)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateBuffer(bufferCreateInfo, category, residencyClass, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

bool VulkanResourceService::CreateVertexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiVertexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"VertexBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outView = RhiVertexBufferView{
	    .BufferLocation = record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .StrideInBytes = strideInBytes};
	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return true;
}

bool VulkanResourceService::CreateStructuredBuffer(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiResourceViewHandle& outView)
{
	outResource = {};
	outView = {};
	if (m_descriptorManager == nullptr || !CreateStructuredBufferResource(data, sizeInBytes, strideInBytes, debugName, outResource))
	{
		return false;
	}

	outView = m_descriptorManager->CreateResourceView(
	    RhiResourceViewDesc::BufferShaderResource(GetNativeResource(outResource), sizeInBytes, strideInBytes));
	if (!outView)
	{
		ReleaseOwnedResource(outResource);
		outResource = {};
		return false;
	}

	return true;
}

bool VulkanResourceService::CreateStructuredBufferResource(
    const void* data,
    std::size_t sizeInBytes,
    std::uint32_t strideInBytes,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource)
{
	outResource = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0 || strideInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes, .StrideInBytes = strideInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"StructuredBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return static_cast<bool>(outResource);
}

bool VulkanResourceService::CreateIndexBuffer(
    const void* data,
    std::size_t sizeInBytes,
    RhiIndexFormat format,
    std::wstring_view debugName,
    RhiOwnedResourceHandle& outResource,
    RhiIndexBufferView& outView)
{
	outResource = {};
	outView = {};
	if (m_memoryAllocator == nullptr || data == nullptr || sizeInBytes == 0)
	{
		return false;
	}

	const RhiBufferResourceDesc desc{.SizeInBytes = sizeInBytes};
	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(
	    desc,
	    VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
	        VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
	std::unique_ptr<VulkanGpuAllocationRecord> record = m_memoryAllocator->CreateBuffer(
	    bufferCreateInfo,
	    RhiMemoryCategory::Mesh,
	    RhiMemoryResidencyClass::HostUpload,
	    debugName.empty() ? L"IndexBuffer" : debugName);
	if (record == nullptr || record->Buffer == VK_NULL_HANDLE || !m_memoryAllocator->WriteAllocation(*record, data, sizeInBytes))
	{
		return false;
	}

	outView = RhiIndexBufferView{
	    .BufferLocation = record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer),
	    .SizeInBytes = static_cast<std::uint32_t>(sizeInBytes),
	    .Format = format};
	outResource = MakeVulkanOwnedResourceHandle(std::move(record));
	return true;
}

void VulkanResourceService::ReleaseOwnedResource(RhiOwnedResourceHandle resource) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	std::unique_ptr<VulkanGpuAllocationRecord> record = TakeVulkanOwnedResourceHandle(resource);
	if (record == nullptr)
	{
		return;
	}

	const std::uint64_t retireFenceValue = m_commandContext != nullptr ? m_commandContext->GetNextRetireFenceValue() : 0;
	m_memoryAllocator->QueueDestroyResource(std::move(record), retireFenceValue);
	DrainCompletedResourceReleases();
}

void VulkanResourceService::FlushDeferredResourceReleases() noexcept
{
	if (m_memoryAllocator != nullptr)
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

void VulkanResourceService::DrainCompletedResourceReleases() noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	if (m_commandContext != nullptr)
	{
		m_memoryAllocator->DrainCompletedReleases(m_commandContext->GetCompletedRetireFenceValue());
	}
	else
	{
		m_memoryAllocator->FlushPendingReleases();
	}
}

NativeResourceHandle VulkanResourceService::GetNativeResource(RhiOwnedResourceHandle resource) const noexcept
{
	VulkanGpuAllocationRecord* const record = GetVulkanGpuAllocationRecord(resource);
	return record != nullptr ? GetVulkanNativeResource(*record) : NativeResourceHandle{};
}

RhiGpuVirtualAddress VulkanResourceService::GetResourceGpuVirtualAddress(RhiOwnedResourceHandle resource) const noexcept
{
	VulkanGpuAllocationRecord* const record = GetVulkanGpuAllocationRecord(resource);
	if (record == nullptr)
	{
		return 0;
	}
	return record->DeviceAddress != 0 ? record->DeviceAddress : reinterpret_cast<std::uint64_t>(record->Buffer);
}

RhiResourceAllocationInfo VulkanResourceService::GetTextureAllocationInfo(const RhiTextureResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr || desc.Width == 0 || desc.Height == 0 || desc.Format == PixelFormat::Unknown)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc);
	const VkDeviceImageMemoryRequirements requirementsInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_IMAGE_MEMORY_REQUIREMENTS,
	    .pNext = nullptr,
	    .pCreateInfo = &imageCreateInfo,
	    .planeAspect = static_cast<VkImageAspectFlagBits>(0)};
	VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
	vkGetDeviceImageMemoryRequirements(m_rhi->GetDevice(), &requirementsInfo, &memoryRequirements);
	return RhiResourceAllocationInfo{
	    .SizeInBytes = memoryRequirements.memoryRequirements.size,
	    .Alignment = memoryRequirements.memoryRequirements.alignment};
}

RhiResourceAllocationInfo VulkanResourceService::GetBufferAllocationInfo(const RhiBufferResourceDesc& desc) const noexcept
{
	if (m_rhi == nullptr || desc.SizeInBytes == 0)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc);
	const VkDeviceBufferMemoryRequirements requirementsInfo{
	    .sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS,
	    .pNext = nullptr,
	    .pCreateInfo = &bufferCreateInfo};
	VkMemoryRequirements2 memoryRequirements{.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};
	vkGetDeviceBufferMemoryRequirements(m_rhi->GetDevice(), &requirementsInfo, &memoryRequirements);
	return RhiResourceAllocationInfo{
	    .SizeInBytes = memoryRequirements.memoryRequirements.size,
	    .Alignment = memoryRequirements.memoryRequirements.alignment};
}

RhiOwnedMemoryBlockHandle VulkanResourceService::CreateTransientMemoryBlock(
    RhiTransientAllocationPool pool,
    std::uint64_t sizeInBytes,
    std::uint64_t alignment,
    std::wstring_view debugName)
{
	if (m_memoryAllocator == nullptr || sizeInBytes == 0)
	{
		return {};
	}

	std::unique_ptr<VulkanGpuMemoryBlockRecord> record =
	    m_memoryAllocator->CreateTransientMemoryBlock(pool, sizeInBytes, alignment, debugName);
	return record != nullptr ? MakeVulkanOwnedMemoryBlockHandle(std::move(record)) : RhiOwnedMemoryBlockHandle{};
}

void VulkanResourceService::ReleaseTransientMemoryBlock(RhiOwnedMemoryBlockHandle memoryBlock) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	std::unique_ptr<VulkanGpuMemoryBlockRecord> record = TakeVulkanOwnedMemoryBlockHandle(memoryBlock);
	if (record == nullptr)
	{
		return;
	}

	const std::uint64_t retireFenceValue = m_commandContext != nullptr ? m_commandContext->GetNextRetireFenceValue() : 0;
	m_memoryAllocator->QueueDestroyMemoryBlock(std::move(record), retireFenceValue);
	DrainCompletedResourceReleases();
}

RhiOwnedResourceHandle VulkanResourceService::CreateAliasingTextureResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientTextureAllocationDesc& desc,
    std::wstring_view debugName)
{
	(void) desc.InitialState;
	(void) desc.ClearValue;
	if (m_memoryAllocator == nullptr || m_capabilities == nullptr || !memoryBlock ||
	    !RhiContract::IsTextureResourceDescUsable(*m_capabilities, desc.ResourceDesc))
	{
		return {};
	}

	VulkanGpuMemoryBlockRecord* const memoryBlockRecord = GetVulkanGpuMemoryBlockRecord(memoryBlock);
	if (memoryBlockRecord == nullptr)
	{
		return {};
	}

	const VkImageCreateInfo imageCreateInfo = VulkanTypeConversions::BuildTextureCreateInfo(desc.ResourceDesc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateAliasingImage(*memoryBlockRecord, memoryBlockOffset, imageCreateInfo, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

RhiOwnedResourceHandle VulkanResourceService::CreateAliasingBufferResource(
    RhiOwnedMemoryBlockHandle memoryBlock,
    std::uint64_t memoryBlockOffset,
    const RhiTransientBufferAllocationDesc& desc,
    std::wstring_view debugName)
{
	(void) desc.InitialState;
	if (m_memoryAllocator == nullptr || !memoryBlock || desc.ResourceDesc.SizeInBytes == 0)
	{
		return {};
	}

	VulkanGpuMemoryBlockRecord* const memoryBlockRecord = GetVulkanGpuMemoryBlockRecord(memoryBlock);
	if (memoryBlockRecord == nullptr)
	{
		return {};
	}

	const VkBufferCreateInfo bufferCreateInfo = VulkanTypeConversions::BuildBufferCreateInfo(desc.ResourceDesc);
	std::unique_ptr<VulkanGpuAllocationRecord> record =
	    m_memoryAllocator->CreateAliasingBuffer(*memoryBlockRecord, memoryBlockOffset, bufferCreateInfo, debugName);
	return record != nullptr ? MakeVulkanOwnedResourceHandle(std::move(record)) : RhiOwnedResourceHandle{};
}

bool VulkanResourceService::SupportsUnorderedAccess(NativeResourceHandle) const noexcept
{
	return false;
}
