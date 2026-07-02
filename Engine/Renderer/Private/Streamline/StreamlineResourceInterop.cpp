#include "../PCH.h"
#include "Streamline/StreamlineResourceInterop.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
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

sl::Resource BuildStreamlineTextureResource(
    ERhiBackendApi backendApi,
    NativeResourceHandle resourceHandle,
    const NativeTextureViewInfo& view,
    std::uint32_t d3d12State) noexcept
{
	if (backendApi != ERhiBackendApi::Vulkan)
	{
		return sl::Resource{sl::ResourceType::eTex2d, resourceHandle.Value, d3d12State};
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
#endif
