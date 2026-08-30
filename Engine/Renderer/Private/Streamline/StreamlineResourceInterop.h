#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl.h>

// Keeps a Streamline resource and its optional Vulkan subresource range in one
// stable object so feature-specific tag builders do not duplicate backend rules.
class StreamlineTaggedTextureResource final
{
public:
	StreamlineTaggedTextureResource(ERhiBackendApi backendApi, const NativeTextureViewInfo& view) noexcept;

	StreamlineTaggedTextureResource(const StreamlineTaggedTextureResource&) = delete;
	StreamlineTaggedTextureResource& operator=(const StreamlineTaggedTextureResource&) = delete;
	StreamlineTaggedTextureResource(StreamlineTaggedTextureResource&&) = delete;
	StreamlineTaggedTextureResource& operator=(StreamlineTaggedTextureResource&&) = delete;

	sl::Resource* Get() noexcept { return &m_resource; }

private:
	sl::Resource m_resource = {};
	sl::SubresourceRange m_subresourceRange = {};
};

bool IsStreamlineTextureViewValid(ERhiBackendApi backendApi, const NativeTextureViewInfo& view) noexcept;

template <typename... TViews> bool AreStreamlineTextureViewsValid(ERhiBackendApi backendApi, const TViews&... views) noexcept
{
	return (IsStreamlineTextureViewValid(backendApi, views) && ...);
}

sl::Extent BuildStreamlineExtent(RenderViewportExtent extent) noexcept;
#endif
