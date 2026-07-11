#pragma once

#include "RHI/Public/Core/RhiCapabilities.h"
#include "RHI/Public/Interop/RhiInteropService.h"
#include "RHI/Public/Interop/RhiExternalFeatureHooks.h"

bool InitializeSharedStreamlineRuntime(ERhiBackendApi backendApi);
RhiExternalFeatureHooks GetSharedStreamlineRhiHooks() noexcept;
void ShutdownSharedStreamlineRuntime() noexcept;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	#include <sl.h>

bool IsStreamlineFeatureSupported(
    sl::Feature feature,
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop) noexcept;
#endif
