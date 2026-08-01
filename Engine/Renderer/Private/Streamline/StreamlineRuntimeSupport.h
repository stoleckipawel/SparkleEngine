#pragma once

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiD3D12InterposerHooks.h"

bool InitializeSharedStreamlineRuntime(ERhiBackendApi backendApi);
RhiD3D12InterposerHooks GetSharedStreamlineD3D12InterposerHooks() noexcept;
void ShutdownSharedStreamlineRuntime() noexcept;
void SetSharedStreamlineFrameMarker(ERhiFrameLatencyMarker marker, std::uint64_t frameId) noexcept;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include <sl.h>

bool IsStreamlineFeatureSupported(
    sl::Feature feature,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept;
#endif
