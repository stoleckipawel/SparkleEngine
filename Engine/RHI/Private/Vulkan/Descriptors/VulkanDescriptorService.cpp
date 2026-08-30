#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Descriptors/VulkanDescriptorService.h"

#include "Vulkan/Core/VulkanResult.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Memory/VulkanRecordingResource.h"
#include "Vulkan/Samplers/VulkanSamplerLibrary.h"
#include "Vulkan/SwapChain/VulkanSwapChain.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Validation/RhiContract.h"

#include <algorithm>
#include <utility>

static const auto g_vulkanDescriptorServiceLogger = Logging::GetOrCreateLogger("RHI.Vulkan.DescriptorService");

bool VulkanDescriptorService::ResourceViewRecord::IsAllocated() const noexcept
{
	return Image != VK_NULL_HANDLE || Buffer != VK_NULL_HANDLE || AccelerationStructure != VK_NULL_HANDLE || ImageView != VK_NULL_HANDLE
	    || static_cast<bool>(DescriptorHandle);
}

VulkanDescriptorService::VulkanDescriptorService(
    VulkanRhi& rhi,
    VulkanGpuMemoryAllocator& memoryAllocator,
    const RhiCapabilities& capabilities) noexcept :
    m_rhi(rhi),
    m_memoryAllocator(memoryAllocator),
    m_capabilities(capabilities),
    m_allocator(rhi)
{
}

VulkanDescriptorService::~VulkanDescriptorService() noexcept
{
	ReleaseAllResourceViews();
}

void VulkanDescriptorService::BeginFrame(std::uint32_t frameIndex) noexcept
{
	if (frameIndex >= m_retiredResourceViews.size())
	{
		Diagnostics::Fatal(g_vulkanDescriptorServiceLogger, __FILE__, __LINE__, "Vulkan descriptor frame index is out of range.");
	}

	for (RetiredResourceView& retired : m_retiredResourceViews[frameIndex])
	{
		DestroyResourceView(retired.Record);
		RecycleResourceViewRecord(retired.RecordIndex);
	}
	m_retiredResourceViews[frameIndex].clear();
	m_currentFrameIndex = frameIndex;
	m_allocator.BeginFrame(frameIndex);
}

void VulkanDescriptorService::SetSamplerLibrary(VulkanSamplerLibrary& samplerLibrary) noexcept
{
	m_samplerLibrary = &samplerLibrary;
}

std::unique_ptr<RenderBindingSet> VulkanDescriptorService::CreateBindingSet(const RenderBindingSetDesc& desc)
{
	return std::make_unique<RenderBindingSet>(m_capabilities, *this, desc);
}

void VulkanDescriptorService::BindGlobalDescriptorState(RenderCommandList&) const noexcept
{
}

void VulkanDescriptorService::PublishRecordingReadView() noexcept
{
	auto readView = std::make_shared<RecordingReadView>();
	readView->ImageViews.reserve(m_resourceViewRecords.size());
	readView->ImageResources.reserve(m_resourceViewRecords.size());
	for (const ResourceViewRecord& record : m_resourceViewRecords)
	{
		if (record.ImageView == VK_NULL_HANDLE)
		{
			continue;
		}

		const RhiResourceViewDesc viewDesc{
		    .Kind = record.Kind,
		    .Resource = RhiResourceHandle{record.Image},
		    .Format = record.Format,
		    .Texture = record.Texture};
		readView->ImageViews.push_back(
		    RecordingImageView{
		        .ImageViewValue = reinterpret_cast<std::uintptr_t>(record.ImageView),
		        .AspectMask = ResolveViewAspectMask(viewDesc)});
		if (record.Image != VK_NULL_HANDLE)
		{
			const VulkanGpuAllocationRecord* const allocation = m_memoryAllocator.FindAllocationRecord(RhiResourceHandle{record.Image});
			readView->ImageResources.push_back(
			    RecordingImageResource{
			        .ResourceHandleValue = reinterpret_cast<std::uintptr_t>(record.Image),
			        .Image = record.Image,
			        .Format = allocation != nullptr ? allocation->Format : VulkanTypeConversions::ToVkFormat(record.Format),
			        .Extent = allocation != nullptr ? allocation->Extent : record.Extent,
			        .AspectMask = ResolveViewAspectMask(viewDesc),
			        .Usage = allocation != nullptr ? allocation->Usage : record.Usage});
		}
	}
	std::ranges::sort(readView->ImageViews, {}, &RecordingImageView::ImageViewValue);
	std::ranges::sort(readView->ImageResources, {}, &RecordingImageResource::ResourceHandleValue);
	m_recordingReadView.store(std::shared_ptr<const RecordingReadView>(std::move(readView)), std::memory_order_release);

	m_allocator.PublishRecordingReadView();
}

RhiDescriptorAllocation VulkanDescriptorService::AllocateDescriptor(ERhiDescriptorAllocatorType descriptorType)
{
	return m_allocator.AllocateDescriptor(descriptorType);
}

void VulkanDescriptorService::ReleaseDescriptor(
    ERhiDescriptorAllocatorType descriptorType,
    const RhiDescriptorAllocation& allocation) noexcept
{
	m_allocator.ReleaseDescriptor(descriptorType, allocation);
}

RhiDescriptorTableHandle VulkanDescriptorService::AllocateDescriptorTable(
    ERhiDescriptorAllocatorType descriptorType,
    std::uint32_t descriptorCount)
{
	return m_allocator.AllocateDescriptorTable(descriptorType, descriptorCount);
}

RhiCpuDescriptorHandle VulkanDescriptorService::GetDescriptorTableCpuHandle(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex) const noexcept
{
	return m_allocator.GetDescriptorTableCpuHandle(tableHandle, descriptorIndex);
}

void VulkanDescriptorService::ReleaseDescriptorTable(RhiDescriptorTableHandle tableHandle) noexcept
{
	m_allocator.ReleaseDescriptorTable(tableHandle);
}

void VulkanDescriptorService::WriteSamplerDescriptor(RhiDescriptorTableHandle table, VkSampler sampler) noexcept
{
	m_allocator.WriteSamplerDescriptor(GetDescriptorTableCpuHandle(table), sampler);
}

RhiDescriptorTableBinding VulkanDescriptorService::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	if (m_samplerLibrary == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanDescriptorServiceLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan shared sampler binding was requested before sampler-library initialization.");
	}
	return m_samplerLibrary->GetSharedSamplerBinding(samplerDesc);
}

RhiResourceViewHandle VulkanDescriptorService::CreateResourceView(const RhiResourceViewDesc& desc)
{
	if (!RhiContract::IsResourceViewDescUsable(desc))
	{
		return {};
	}

	switch (desc.Kind)
	{
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
			return CreateTextureDescriptorView(desc);
		case ERhiResourceViewKind::RenderTarget:
		case ERhiResourceViewKind::DepthStencil:
			return CreateAttachmentView(desc);
		case ERhiResourceViewKind::BufferShaderResource:
		case ERhiResourceViewKind::BufferUnorderedAccess:
			return CreateBufferDescriptorView(desc);
	}
	Diagnostics::Fatal(g_vulkanDescriptorServiceLogger, __FILE__, __LINE__, "Vulkan resource view uses an unknown view kind.");
}

RhiResourceViewHandle VulkanDescriptorService::CreateTextureDescriptorView(const RhiResourceViewDesc& desc)
{
	const VkImageView imageView = CreateImageView(desc);
	if (imageView == VK_NULL_HANDLE)
	{
		return {};
	}
	const RhiGpuDescriptorHandle descriptorHandle = m_allocator.RegisterImageDescriptor(desc.Kind, imageView);
	if (!descriptorHandle)
	{
		vkDestroyImageView(m_rhi.GetDevice(), imageView, nullptr);
		return {};
	}
	return AddResourceView(
	    ResourceViewRecord{
	        .Kind = desc.Kind,
	        .Image = static_cast<VkImage>(desc.Resource.Value),
	        .ImageView = imageView,
	        .Format = desc.Format,
	        .Texture = desc.Texture,
	        .Usage = desc.Kind == ERhiResourceViewKind::TextureUnorderedAccess ? static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_STORAGE_BIT)
	                                                                           : static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_SAMPLED_BIT),
	        .DescriptorHandle = descriptorHandle,
	        .OwnsImageView = true});
}

RhiResourceViewHandle VulkanDescriptorService::CreateAttachmentView(const RhiResourceViewDesc& desc)
{
	const VkImageView imageView = CreateImageView(desc);
	if (imageView == VK_NULL_HANDLE)
	{
		return {};
	}
	return AddResourceView(
	    ResourceViewRecord{
	        .Kind = desc.Kind,
	        .Image = static_cast<VkImage>(desc.Resource.Value),
	        .ImageView = imageView,
	        .Format = desc.Format,
	        .Texture = desc.Texture,
	        .Usage = desc.Kind == ERhiResourceViewKind::DepthStencil
	            ? static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	            : static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
	        .OwnsImageView = true});
}

RhiResourceViewHandle VulkanDescriptorService::CreateBufferDescriptorView(const RhiResourceViewDesc& desc)
{
	const VkBuffer buffer = static_cast<VkBuffer>(desc.Resource.Value);
	const RhiGpuDescriptorHandle descriptorHandle =
	    m_allocator.RegisterBufferDescriptor(desc.Kind, buffer, desc.Buffer.OffsetInBytes, desc.Buffer.SizeInBytes);
	if (!descriptorHandle)
	{
		return {};
	}
	return AddResourceView(ResourceViewRecord{.Kind = desc.Kind, .Buffer = buffer, .DescriptorHandle = descriptorHandle});
}

void VulkanDescriptorService::ReleaseResourceView(RhiResourceViewHandle view) noexcept
{
	ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr)
	{
		return;
	}

	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation))
	{
		return;
	}

	m_retiredResourceViews[m_currentFrameIndex].push_back(RetiredResourceView{.Record = *record, .RecordIndex = recordIndex});
	const std::uint16_t preservedGeneration = record->Generation;
	*record = {};
	record->Generation = preservedGeneration;
}

bool VulkanDescriptorService::WriteResourceView(
    RhiDescriptorTableHandle tableHandle,
    std::uint32_t descriptorIndex,
    RhiResourceViewHandle view) noexcept
{
	const ResourceViewRecord* const resourceView = FindResourceViewRecord(view);
	if (resourceView == nullptr || !resourceView->DescriptorHandle)
	{
		return false;
	}

	const RhiCpuDescriptorHandle destination = m_allocator.GetDescriptorTableCpuHandle(tableHandle, descriptorIndex);
	return destination && m_allocator.WriteRegisteredDescriptor(destination, resourceView->DescriptorHandle);
}

RhiCpuDescriptorHandle VulkanDescriptorService::GetResourceViewCpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? VulkanDescriptorHandles::MakeImageViewCpuHandle(record->ImageView) : RhiCpuDescriptorHandle{};
}

RhiGpuDescriptorHandle VulkanDescriptorService::GetResourceViewGpuHandle(RhiResourceViewHandle view) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	return record != nullptr ? record->DescriptorHandle : RhiGpuDescriptorHandle{};
}

NativeTextureViewInfo VulkanDescriptorService::ResolveNativeTextureViewInfo(
    RhiResourceViewHandle view,
    RhiResourceHandle,
    ResourceState state) const noexcept
{
	const ResourceViewRecord* const record = FindResourceViewRecord(view);
	if (record == nullptr || record->Image == VK_NULL_HANDLE || record->ImageView == VK_NULL_HANDLE)
	{
		return {};
	}

	const VulkanGpuAllocationRecord* const allocation = m_memoryAllocator.FindAllocationRecord(RhiResourceHandle{record->Image});
	const VulkanResourceStateMapping stateMapping = VulkanTypeConversions::ToResourceStateMapping(state);
	const RhiResourceViewDesc viewDesc{
	    .Kind = record->Kind,
	    .Resource = RhiResourceHandle{record->Image},
	    .Format = record->Format,
	    .Texture = record->Texture};
	const VkImageAspectFlags aspectMask = ResolveViewAspectMask(viewDesc);
	const VkExtent3D extent = allocation != nullptr ? allocation->Extent : record->Extent;
	const VkFormat format = allocation != nullptr && allocation->Format != VK_FORMAT_UNDEFINED
	    ? allocation->Format
	    : VulkanTypeConversions::ToVkFormat(record->Format);
	const VkImageUsageFlags allocationUsage = allocation != nullptr ? allocation->Usage : 0;
	const VkImageUsageFlags usage = allocationUsage != 0 ? allocationUsage : record->Usage;
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
	    .NativeUsage = static_cast<std::uint32_t>(usage)};
}

VkImageView VulkanDescriptorService::GetRegisteredImageView(RhiGpuDescriptorHandle descriptorHandle) const noexcept
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

VkImageAspectFlags VulkanDescriptorService::ResolveImageViewAspectMask(VkImageView imageView) const noexcept
{
	if (imageView == VK_NULL_HANDLE)
	{
		return 0;
	}

	const std::shared_ptr<const RecordingReadView> readView = m_recordingReadView.load(std::memory_order_acquire);
	if (readView == nullptr)
	{
		return 0;
	}

	const std::uintptr_t imageViewValue = reinterpret_cast<std::uintptr_t>(imageView);
	const auto found = std::ranges::lower_bound(readView->ImageViews, imageViewValue, {}, &RecordingImageView::ImageViewValue);
	return found != readView->ImageViews.end() && found->ImageViewValue == imageViewValue ? found->AspectMask : 0;
}

bool VulkanDescriptorService::ResolveRegisteredImageResource(
    RhiResourceHandle resource,
    VulkanRecordingResource& outResource) const noexcept
{
	if (!resource)
	{
		return false;
	}

	const std::shared_ptr<const RecordingReadView> readView = m_recordingReadView.load(std::memory_order_acquire);
	if (readView == nullptr)
	{
		return false;
	}

	const std::uintptr_t resourceHandleValue = reinterpret_cast<std::uintptr_t>(resource.Value);
	auto found = std::ranges::lower_bound(readView->ImageResources, resourceHandleValue, {}, &RecordingImageResource::ResourceHandleValue);
	if (found == readView->ImageResources.end() || found->ResourceHandleValue != resourceHandleValue)
	{
		return false;
	}

	VkImageAspectFlags aspectMask = 0;
	const VkImage image = found->Image;
	const VkFormat format = found->Format;
	const VkExtent3D extent = found->Extent;
	const VkImageUsageFlags usage = found->Usage;
	for (; found != readView->ImageResources.end() && found->ResourceHandleValue == resourceHandleValue; ++found)
	{
		aspectMask |= found->AspectMask;
	}
	if (image == VK_NULL_HANDLE || aspectMask == 0)
	{
		return false;
	}

	outResource = VulkanRecordingResource{
	    .Image = image,
	    .ResourceHandleValue = resourceHandleValue,
	    .Format = format,
	    .Extent = extent,
	    .AspectMask = aspectMask,
	    .Usage = usage,
	    .ResourceKind = VulkanGpuAllocationResourceKind::Image};
	return true;
}

void VulkanDescriptorService::RebuildSwapChainBackBufferViews(const VulkanSwapChain& swapChain) noexcept
{
	for (const RhiResourceViewHandle view : m_swapChainBackBufferViews)
	{
		ReleaseResourceView(view);
	}
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
		        .Format = swapChain.GetBackBufferFormat(),
		        .Extent = swapChain.GetBackBufferExtent(),
		        .Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		        .OwnsImageView = false}));
	}
}

RhiResourceViewHandle VulkanDescriptorService::GetSwapChainBackBufferView(std::uint32_t backBufferIndex) const noexcept
{
	return backBufferIndex < m_swapChainBackBufferViews.size() ? m_swapChainBackBufferViews[backBufferIndex] : RhiResourceViewHandle{};
}

void VulkanDescriptorService::ReleaseAllResourceViews() noexcept
{
	for (ResourceViewRecord& record : m_resourceViewRecords)
	{
		DestroyResourceView(record);
	}
	for (auto& retiredViews : m_retiredResourceViews)
	{
		for (RetiredResourceView& retired : retiredViews)
		{
			DestroyResourceView(retired.Record);
		}
		retiredViews.clear();
	}
	m_freeResourceViewIndices.clear();
	m_swapChainBackBufferViews.clear();
}

void VulkanDescriptorService::DestroyResourceView(ResourceViewRecord& record) noexcept
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

RhiResourceViewHandle VulkanDescriptorService::AddResourceView(ResourceViewRecord record)
{
	if (!m_freeResourceViewIndices.empty())
	{
		const std::uint32_t index = m_freeResourceViewIndices.back();
		m_freeResourceViewIndices.pop_back();
		record.Generation = m_resourceViewRecords[index].Generation;
		m_resourceViewRecords[index] = record;
		return RhiResourceViewHandle::Make(index, record.Generation);
	}

	if (m_resourceViewRecords.size() >= RhiResourceViewHandle::MaximumRecordCount)
	{
		DestroyResourceView(record);
		return {};
	}
	m_resourceViewRecords.push_back(record);
	return RhiResourceViewHandle::Make(static_cast<std::uint32_t>(m_resourceViewRecords.size() - 1u), 0u);
}

VulkanDescriptorService::ResourceViewRecord* VulkanDescriptorService::FindResourceViewRecord(RhiResourceViewHandle view) noexcept
{
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation) || recordIndex >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	return record.IsAllocated() && record.Generation == generation ? &record : nullptr;
}

const VulkanDescriptorService::ResourceViewRecord* VulkanDescriptorService::FindResourceViewRecord(
    RhiResourceViewHandle view) const noexcept
{
	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation) || recordIndex >= m_resourceViewRecords.size())
	{
		return nullptr;
	}
	const ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	return record.IsAllocated() && record.Generation == generation ? &record : nullptr;
}

void VulkanDescriptorService::RecycleResourceViewRecord(std::uint32_t recordIndex) noexcept
{
	if (recordIndex >= m_resourceViewRecords.size())
	{
		return;
	}

	ResourceViewRecord& record = m_resourceViewRecords[recordIndex];
	if (record.IsAllocated() || record.Generation == RhiResourceViewHandle::MaximumGeneration)
	{
		return;
	}

	++record.Generation;
	m_freeResourceViewIndices.push_back(recordIndex);
}

VkImageView VulkanDescriptorService::CreateImageView(const RhiResourceViewDesc& desc) const
{
	const VkImageViewCreateInfo createInfo{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .image = static_cast<VkImage>(desc.Resource.Value),
	    .viewType = desc.TextureDimension == TextureResourceDimension::TextureCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D,
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
		Diagnostics::Fatal(g_vulkanDescriptorServiceLogger, __FILE__, __LINE__, VulkanResult::FormatFailure("vkCreateImageView", result));
	}
	return imageView;
}

VkFormat VulkanDescriptorService::ResolveViewFormat(const RhiResourceViewDesc& desc) const noexcept
{
	return desc.Format != PixelFormat::Unknown ? VulkanTypeConversions::ToVkFormat(desc.Format) : VK_FORMAT_UNDEFINED;
}

VkImageAspectFlags VulkanDescriptorService::ResolveViewAspectMask(const RhiResourceViewDesc& desc) const noexcept
{
	switch (desc.Kind)
	{
		case ERhiResourceViewKind::RenderTarget:
			return VK_IMAGE_ASPECT_COLOR_BIT;
		case ERhiResourceViewKind::DepthStencil:
			return VulkanTypeConversions::ResolveAspectMask(desc.Format);
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
			if (desc.Format == PixelFormat::D32_Float || desc.Format == PixelFormat::D24_UNorm_S8_UInt)
			{
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			}
			if (desc.Format == PixelFormat::Unknown)
			{
				Diagnostics::Fatal(
				    g_vulkanDescriptorServiceLogger,
				    __FILE__,
				    __LINE__,
				    "Vulkan texture views require an explicit format when resolving their image aspect.");
			}
			return VK_IMAGE_ASPECT_COLOR_BIT;
		default:
			Diagnostics::Fatal(
			    g_vulkanDescriptorServiceLogger,
			    __FILE__,
			    __LINE__,
			    "Cannot resolve an image aspect for a non-image Vulkan resource view.");
	}
}
