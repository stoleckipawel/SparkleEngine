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
#include "Validation/RhiContract.h"

#include <algorithm>
#include <utility>

static const auto g_vulkanDescriptorManagerLogger = Logging::GetOrCreateLogger("RHI.Vulkan.DescriptorManager");

bool VulkanDescriptorManager::ResourceViewRecord::IsAllocated() const noexcept
{
	return Image != VK_NULL_HANDLE || Buffer != VK_NULL_HANDLE || AccelerationStructure != VK_NULL_HANDLE ||
	       ImageView != VK_NULL_HANDLE || static_cast<bool>(DescriptorHandle);
}

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
	if (frameIndex >= m_retiredResourceViews.size())
	{
		return;
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

void VulkanDescriptorManager::PublishRecordingReadView() noexcept
{
	auto readView = std::make_shared<RecordingReadView>();
	readView->ImageViews.reserve(m_resourceViewRecords.size());
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
	}
	std::ranges::sort(
	    readView->ImageViews,
	    {},
	    &RecordingImageView::ImageViewValue);
	std::atomic_store(
	    &m_recordingReadView,
	    std::shared_ptr<const RecordingReadView>(std::move(readView)));

	m_allocator.PublishRecordingReadView();
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

RhiDescriptorTableBinding VulkanDescriptorManager::GetSharedSamplerBinding(const RhiSamplerDesc& samplerDesc) const noexcept
{
	return m_samplerLibrary != nullptr ? m_samplerLibrary->GetSharedSamplerBinding(samplerDesc) : RhiDescriptorTableBinding{};
}

RhiResourceViewHandle VulkanDescriptorManager::CreateResourceView(const RhiResourceViewDesc& desc)
{
	if (!RhiContract::IsResourceViewDescUsable(desc))
	{
		return {};
	}

	switch (desc.Kind)
	{
		case ERhiResourceViewKind::TextureShaderResource:
		case ERhiResourceViewKind::TextureUnorderedAccess:
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
			        .Usage = desc.Kind == ERhiResourceViewKind::TextureUnorderedAccess ?
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_STORAGE_BIT) :
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_SAMPLED_BIT),
			        .DescriptorHandle = descriptorHandle,
			        .OwnsImageView = true});
		}
		case ERhiResourceViewKind::RenderTarget:
		case ERhiResourceViewKind::DepthStencil:
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
			        .Usage = desc.Kind == ERhiResourceViewKind::DepthStencil ?
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) :
			                     static_cast<VkImageUsageFlags>(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT),
			        .OwnsImageView = true});
		}
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

	std::uint32_t recordIndex = 0;
	std::uint16_t generation = 0;
	if (!view.Decode(recordIndex, generation))
	{
		return;
	}

	m_retiredResourceViews[m_currentFrameIndex].push_back(
	    RetiredResourceView{.Record = *record, .RecordIndex = recordIndex});
	const std::uint16_t preservedGeneration = record->Generation;
	*record = {};
	record->Generation = preservedGeneration;
}

bool VulkanDescriptorManager::WriteResourceView(
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

NativeTextureViewInfo VulkanDescriptorManager::ResolveNativeTextureViewInfo(
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
	const VkExtent3D extent = allocation != nullptr ? allocation->Extent : VkExtent3D{};
	const VkFormat format = allocation != nullptr && allocation->Format != VK_FORMAT_UNDEFINED ?
	                            allocation->Format :
	                            VulkanTypeConversions::ToVkFormat(record->Format);
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

	const std::shared_ptr<const RecordingReadView> readView =
	    std::atomic_load(&m_recordingReadView);
	if (readView == nullptr)
	{
		return VK_IMAGE_ASPECT_COLOR_BIT;
	}

	const std::uintptr_t imageViewValue =
	    reinterpret_cast<std::uintptr_t>(imageView);
	const auto found = std::ranges::lower_bound(
	    readView->ImageViews,
	    imageViewValue,
	    {},
	    &RecordingImageView::ImageViewValue);
	return found != readView->ImageViews.end() &&
	               found->ImageViewValue == imageViewValue
	           ? found->AspectMask
	           : VK_IMAGE_ASPECT_COLOR_BIT;
}

void VulkanDescriptorManager::RebuildSwapChainBackBufferViews(const VulkanSwapChain& swapChain) noexcept
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

void VulkanDescriptorManager::DestroyResourceView(ResourceViewRecord& record) noexcept
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

RhiResourceViewHandle VulkanDescriptorManager::AddResourceView(ResourceViewRecord record)
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

VulkanDescriptorManager::ResourceViewRecord* VulkanDescriptorManager::FindResourceViewRecord(RhiResourceViewHandle view) noexcept
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

const VulkanDescriptorManager::ResourceViewRecord* VulkanDescriptorManager::FindResourceViewRecord(RhiResourceViewHandle view) const noexcept
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

void VulkanDescriptorManager::RecycleResourceViewRecord(std::uint32_t recordIndex) noexcept
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

VkImageView VulkanDescriptorManager::CreateImageView(const RhiResourceViewDesc& desc) const
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
