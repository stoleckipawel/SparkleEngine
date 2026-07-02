#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Interop/RhiNativeHandles.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

sl::SubresourceRange BuildStreamlineSubresourceRange(const NativeTextureViewInfo& view) noexcept;
sl::Resource BuildStreamlineTextureResource(
    ERhiBackendApi backendApi,
    NativeResourceHandle resource,
    const NativeTextureViewInfo& view,
    std::uint32_t d3d12State) noexcept;
#endif
