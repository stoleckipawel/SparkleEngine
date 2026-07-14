#include "../PCH.h"
#include "Streamline/StreamlineResourceInterop.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
namespace
{
	sl::Resource BuildStreamlineTextureResource(
	    ERhiBackendApi backendApi,
	    const NativeTextureViewInfo& view) noexcept
	{
		if (backendApi != ERhiBackendApi::Vulkan)
		{
			return sl::Resource{sl::ResourceType::eTex2d, view.Resource.Value, view.NativeState};
		}

		sl::Resource resource{
		    sl::ResourceType::eTex2d,
		    view.Resource.Value,
		    nullptr,
		    view.View.Value,
		    view.NativeState};
		resource.width = view.Width;
		resource.height = view.Height;
		resource.nativeFormat = view.NativeFormat;
		resource.mipLevels = view.MipLevels;
		resource.arrayLayers = view.ArrayLayers;
		resource.flags = view.NativeFlags;
		resource.usage = view.NativeUsage;
		return resource;
	}

	sl::SubresourceRange BuildStreamlineSubresourceRange(const NativeTextureViewInfo& view) noexcept
	{
		sl::SubresourceRange range{};
		range.aspectMask = view.SubresourceAspectMask;
		range.baseMipLevel = view.SubresourceBaseMipLevel;
		range.levelCount = view.SubresourceLevelCount;
		range.baseArrayLayer = view.SubresourceBaseArrayLayer;
		range.layerCount = view.SubresourceLayerCount;
		return range;
	}
}

StreamlineTaggedTextureResource::StreamlineTaggedTextureResource(
    ERhiBackendApi backendApi,
    const NativeTextureViewInfo& view) noexcept :
	m_resource(BuildStreamlineTextureResource(backendApi, view)),
    m_subresourceRange(BuildStreamlineSubresourceRange(view))
{
	if (backendApi == ERhiBackendApi::Vulkan)
	{
		m_resource.next = &m_subresourceRange;
	}
}

sl::Extent BuildStreamlineExtent(RenderViewportExtent extent) noexcept
{
	return sl::Extent{.top = 0, .left = 0, .width = extent.Width, .height = extent.Height};
}

bool IsStreamlineTextureViewValid(
    ERhiBackendApi backendApi,
    const NativeTextureViewInfo& view) noexcept
{
	return backendApi == ERhiBackendApi::Vulkan ? static_cast<bool>(view) : static_cast<bool>(view.Resource);
}
#endif
