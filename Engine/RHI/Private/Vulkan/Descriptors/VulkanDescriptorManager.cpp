#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorManager.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Samplers/VulkanSamplerLibrary.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <utility>

static const auto g_vulkanDescriptorManagerLogger = Logging::GetOrCreateLogger("RHI.Vulkan.DescriptorManager");

VulkanDescriptorManager::VulkanDescriptorManager(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    const RhiCapabilities& capabilities) noexcept :
    m_rhi(rhi), m_memoryAllocator(memoryAllocator), m_capabilities(capabilities), m_allocator(rhi)
{
}

VulkanDescriptorManager::~VulkanDescriptorManager() noexcept
{
	ReleaseAllResourceViews();
}

void VulkanDescriptorManager::BeginFrame(std::uint32_t frameIndex) noexcept
{
	m_allocator.BeginFrame(frameIndex);
}

void VulkanDescriptorManager::SetSamplerLibrary(VulkanSamplerLibrary& samplerLibrary) noexcept
{
	m_samplerLibrary = &samplerLibrary;
}

std::unique_ptr<RenderBindingSet> VulkanDescriptorManager::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return std::make_unique<RenderBindingSet>(m_capabilities, *this, desc);
}

void VulkanDescriptorManager::BindGlobalDescriptorState(RenderCommandList&) const noexcept
{
}

RhiDescriptorAllocation VulkanDescriptorManager::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	return m_allocator.AllocateDescriptor(descriptorType);
}

void VulkanDescriptorManager::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	m_allocator.ReleaseDescriptor(descriptorType, allocation);
}

RhiDescriptorTableHandle VulkanDescriptorManager::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_allocator.AllocateDescriptorTable(descriptorType, descriptorCount);
}

RhiCpuDescriptorHandle VulkanDescriptorManager::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_allocator.GetDescriptorTableCpuHandle(tableHandle, descriptorIndex);
}

void VulkanDescriptorManager::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	m_allocator.ReleaseDescriptorTable(tableHandle);
}

void VulkanDescriptorManager::AllocateShaderResourceDescriptor(
    RhiCpuDescriptorHandle& outCpuHandle,
    RhiGpuDescriptorHandle& outGpuHandle)
{
	const RhiDescriptorAllocation allocation = AllocateDescriptor(ERhiDescriptorAllocatorType::ShaderResource);
	outCpuHandle = allocation.CpuHandle;
	outGpuHandle = allocation.GpuHandle;
}

void VulkanDescriptorManager::ReleaseShaderResourceDescriptor(
    RhiCpuDescriptorHandle cpuHandle,
    RhiGpuDescriptorHandle gpuHandle) noexcept
{
	ReleaseDescriptor(
	    ERhiDescriptorAllocatorType::ShaderResource,
	    RhiDescriptorAllocation{.CpuHandle = cpuHandle, .GpuHandle = gpuHandle});
}

RhiDescriptorTableBinding VulkanDescriptorManager::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	return m_samplerLibrary != nullptr ? m_samplerLibrary->GetSharedSamplerBinding(samplerDesc) : RhiDescriptorTableBinding{};
}

RhiResourceViewHandle VulkanDescriptorManager::CreateResourceView(const RhiResourceViewDesc& desc)
{
	if (!desc.Resource && desc.Kind != ERhiResourceViewKind::AccelerationStructureShaderResource)
	{
		return {};
	}

	switch (desc.Kind)
	{
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
		{
			const VkImageView imageView = CreateImageView(desc);
			RhiGpuDescriptorHandle descriptorHandle = {};
			if (imageView != VK_NULL_HANDLE)
			{
				descriptorHandle = m_allocator.RegisterImageDescriptor(desc.Kind, imageView);
			}
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Image = static_cast<VkImage>(desc.Resource.Value),
			        .ImageView = imageView,
			        .Format = desc.Format,
			        .Texture = desc.Texture,
			        .Usage = desc.Kind == ERhiResourceViewKind::TextureUnorderedAccess ?
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_STORAGE_BIT) :
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_SAMPLED_BIT),
			        .DescriptorHandle = descriptorHandle,
			        .OwnsImageView = true});
		}
		case ERhiResourceViewKind::RenderTarget:
		case ERhiResourceViewKind::DepthStencil:
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Image = static_cast<VkImage>(desc.Resource.Value),
			        .ImageView = CreateImageView(desc),
			        .Format = desc.Format,
			        .Texture = desc.Texture,
			        .Usage = desc.Kind == ERhiResourceViewKind::DepthStencil ?
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) :
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
			        .OwnsImageView = true});
		case ERhiResourceViewKind::BufferShaderResource:
		case ERhiResourceViewKind::BufferUnorderedAccess:
		{
			const RhiGpuDescriptorHandle descriptorHandle = m_allocator.RegisterBufferDescriptor(
			    desc.Kind,
			    static_cast<VkBuffer>(desc.Resource.Value),
			    desc.Buffer.OffsetInBytes,
			    desc.Buffer.SizeInBytes != 0 ? desc.Buffer.SizeInBytes : VK_WHOLE_SIZE);
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Buffer = static_cast<VkBuffer>(desc.Resource.Value),
			        .DescriptorHandle = descriptorHandle});
		}
		case ERhiResourceViewKind::AccelerationStructureShaderResource:
		{
			VulkanGpuAllocationRecord* const record =
			    m_memoryAllocator.FindAllocationRecordByDeviceAddress(desc.AccelerationStructureGpuAddress);
			if (record == nullptr ||
			    (record->AccelerationStructure == VK_NULL_HANDLE && !record->IsPartitionedAccelerationStructure))
			{
				return {};
			}

			const RhiGpuDescriptorHandle descriptorHandle =
			    record->IsPartitionedAccelerationStructure ?
			        m_allocator.RegisterPartitionedAccelerationStructureDescriptor(record->DeviceAddress) :
			        m_allocator.RegisterAccelerationStructureDescriptor(record->AccelerationStructure);
			return AddResourceView(
			    ResourceViewRecord{
			        .Kind = desc.Kind,
			        .Buffer = record->Buffer,
			        .AccelerationStructure = record->AccelerationStructure,
			        .DescriptorHandle = descriptorHandle});
		}
		default:
			return {};
	}
}

void VulkanDescriptorManager::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	if (record->OwnsImageView && record->ImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(m_rhi.GetDevice(), record->ImageView, nullptr);
	}
	if (record->DescriptorHandle)
	{
		m_allocator.ReleaseRegisteredDescriptor(record->DescriptorHandle);
	}
	*record = {};
	m_freeResourceViewIndices.push_back(view.Value - 1u);
}

RhiCpuDescriptorHandle VulkanDescriptorManager::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? VulkanDescriptorHandles::MakeImageViewCpuHandle(record->ImageView) : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle VulkanDescriptorManager::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->DescriptorHandle : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo VulkanDescriptorManager::GetNativeTextureViewInfo(RhiResourceViewHandle view, ResourceState state) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr || record->Image == VK_NULL_HANDLE || record->ImageView == VK_NULL_HANDLE)
	{
		return {};
	}

	const VulkanGpuAllocationRecord* const allocation = m_memoryAllocator.FindAllocationRecord(NativeResourceHandle{record->Image});
	const VulkanResourceStateMapping stateMapping = VulkanTypeConversions::ToResourceStateMapping(state);
	const RhiResourceViewDesc viewDesc{
	    .Kind = record->Kind,
	    .Resource = NativeResourceHandle{record->Image},
	    .Format = record->Format,
	    .Texture = record->Texture};
	const VkImageAspectFlags aspectMask = ResolveViewAspectMask(viewDesc);
	const VkExtent3D extent = allocation != nullptr ? allocation->Extent : VkExtent3D{};
	const VkFormat format = allocation != nullptr && allocation->Format != VK_FORMAT_UNDEFINED ?
	                            allocation->Format :
	                            VulkanTypeConversions::ToVkFormat(record->Format);
	const VkImageUsageFlags allocationUsage = allocation != nullptr ? allocation->Usage : 0;
	const VkImageUsageFlags usage = allocationUsage != 0 ? allocationUsage : record->Usage;
	const bool readWrite = (usage & VK_IMAGE_USAGE_STORAGE_BIT) != 0 || record->Kind == ERhiResourceViewKind::TextureUnorderedAccess;

	return NativeTextureViewInfo{
	    .Resource = NativeResourceHandle{record->Image},
	    .View = NativeTextureViewHandle{record->ImageView},
	    .NativeState = static_cast<std::uint32_t>(stateMapping.ImageLayout),
	    .NativeFormat = static_cast<std::uint32_t>(format),
	    .Width = extent.width,
	    .Height = extent.height,
	    .MipLevels = record->Texture.MipCount,
	    .ArrayLayers = record->Texture.ArraySize,
	    .SubresourceAspectMask = static_cast<std::uint32_t>(aspectMask),
	    .SubresourceBaseMipLevel = record->Texture.MostDetailedMip,
	    .SubresourceLevelCount = record->Texture.MipCount,
	    .SubresourceBaseArrayLayer = record->Texture.FirstArraySlice,
	    .SubresourceLayerCount = record->Texture.ArraySize,
	    .NativeFlags = allocation != nullptr ? static_cast<std::uint32_t>(allocation->ImageFlags) : 0u,
	    .NativeUsage = static_cast<std::uint32_t>(usage),
	    .ReadWrite = readWrite};
}

VkImageView VulkanDescriptorManager::GetRegisteredImageView(RhiGpuDescriptorHandle descriptorHandle) const noexcept
{
	if (!descriptorHandle)
	{
		return VK_NULL_HANDLE;
	}

	for (const ResourceViewRecord& record : m_resourceViewRecords)
	{
		if (record.DescriptorHandle.Value == descriptorHandle.Value)
		{
			return record.ImageView;
		}
	}

	return VK_NULL_HANDLE;
}

VkImageAspectFlags VulkanDescriptorManager::ResolveImageViewAspectMask(VkImageView imageView) const noexcept
{
	if (imageView == VK_NULL_HANDLE)
	{
		return 0;
	}

	for (const ResourceViewRecord& record : m_resourceViewRecords)
	{
		if (record.ImageView != imageView)
		{
			continue;
		}

		const RhiResourceViewDesc viewDesc{
		    .Kind = record.Kind,
		    .Resource = NativeResourceHandle{record.Image},
		    .Format = record.Format,
		    .Texture = record.Texture};
		return ResolveViewAspectMask(viewDesc);
	}

	return VK_IMAGE_ASPECT_COLOR_BIT;
}

void VulkanDescriptorManager::RebuildSwapChainBackBufferViews(const VulkanSwapChain& swapChain) noexcept
{
	ReleaseAllResourceViews();
	m_resourceViewRecords.clear();
	m_freeResourceViewIndices.clear();
	m_swapChainBackBufferViews.clear();

	const std::uint32_t backBufferCount = swapChain.GetBackBufferCount();
	m_swapChainBackBufferViews.reserve(backBufferCount);
	for (std::uint32_t backBufferIndex = 0; backBufferIndex < backBufferCount; ++backBufferIndex)
	{
		m_swapChainBackBufferViews.push_back(AddResourceView(
		    ResourceViewRecord{
		        .Kind = ERhiResourceViewKind::RenderTarget,
		        .Image = swapChain.GetBackBufferImage(backBufferIndex),
		        .ImageView = swapChain.GetBackBufferImageView(backBufferIndex),
		        .OwnsImageView = false}));
	}
}

RhiResourceViewHandle VulkanDescriptorManager::GetSwapChainBackBufferView(std::uint32_t backBufferIndex) const noexcept
{
	return backBufferIndex < m_swapChainBackBufferViews.size() ? m_swapChainBackBufferViews[backBufferIndex] : RhiResourceViewHandle{};
}

void VulkanDescriptorManager::ReleaseAllResourceViews() noexcept
{
	for (ResourceViewRecord& record : m_resourceViewRecords)
	{
		if (record.OwnsImageView && record.ImageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(m_rhi.GetDevice(), record.ImageView, nullptr);
		}
		if (record.DescriptorHandle)
		{
			m_allocator.ReleaseRegisteredDescriptor(record.DescriptorHandle);
		}
		record = {};
	}
}

RhiResourceViewHandle VulkanDescriptorManager::MakeResourceViewHandle(std::uint32_t index) noexcept
{
	return RhiResourceViewHandle{index + 1u};
}

RhiResourceViewHandle VulkanDescriptorManager::AddResourceView(ResourceViewRecord record)
{
	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t index = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		m_resourceViewRecords[index] = record;
		return MakeResourceViewHandle(index);
	}

	m_resourceViewRecords.push_back(record);
	return MakeResourceViewHandle(static_cast<std::uint32_t>(m_resourceViewRecords.size() - 1u));
}

VulkanDescriptorManager::ResourceViewRecord* VulkanDescriptorManager::FindResourceViewRecord(RhiResourceViewHandle view) noexcept
{
	if (!view || view.Value - 1u >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	return &m_resourceViewRecords[view.Value - 1u];
}

const VulkanDescriptorManager::ResourceViewRecord* VulkanDescriptorManager::FindResourceViewRecord(RhiResourceViewHandle view) const noexcept
{
	if (!view || view.Value - 1u >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	return &m_resourceViewRecords[view.Value - 1u];
}

VkImageView VulkanDescriptorManager::CreateImageView(const RhiResourceViewDesc& desc) const
{
	const VkImageViewCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .image = static_cast<VkImage>(desc.Resource.Value),
	    .viewType = VK_IMAGE_VIEW_TYPE_2D,
	    .format = ResolveViewFormat(desc),
	    .components =
	        VkComponentMapping{
	            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
	            .a = VK_COMPONENT_SWIZZLE_IDENTITY},
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = ResolveViewAspectMask(desc),
	        .baseMipLevel = desc.Texture.MostDetailedMip,
	        .levelCount = desc.Texture.MipCount,
	        .baseArrayLayer = desc.Texture.FirstArraySlice,
	        .layerCount = desc.Texture.ArraySize}};

	VkImageView imageView = VK_NULL_HANDLE;
	const VkResult result = vkCreateImageView(m_rhi.GetDevice(), &createInfo, nullptr, &imageView);
	if (!VulkanResult::Succeeded(result))
	{
		Diagnostics::Fail(
		    g_vulkanDescriptorManagerLogger,
		    __FILE__,
		    __LINE__,
		    VulkanResult::FormatFailure("vkCreateImageView", result));
	}
	return imageView;
}

VkFormat VulkanDescriptorManager::ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept
{
	return desc.Format != PixelFormat::Unknown ? VulkanTypeConversions::ToVkFormat(desc.Format) : VK_FORMAT_UNDEFINED;
}

VkImageAspectFlags VulkanDescriptorManager::ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept
{
	if (desc.Kind == ERhiResourceViewKind::DepthStencil)
	{
		return VulkanTypeConversions::ResolveAspectMask(desc.Format);
	}

	switch (desc.Format)
	{
		case PixelFormat::D32_Float:
		case PixelFormat::D24_UNorm_S8_UInt:
			return VK_IMAGE_ASPECT_DEPTH_BIT;
		default:
			return VK_IMAGE_ASPECT_COLOR_BIT;
	}
}
