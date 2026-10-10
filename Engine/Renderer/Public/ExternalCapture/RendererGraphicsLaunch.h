#pragma once

#include "RHI/Public/Core/RhiBackendApi.h"
#include "RHI/Public/Diagnostics/RhiExternalCapture.h"

struct RendererGraphicsLaunch final
{
	ERhiBackendApi BackendApi = ERhiBackendApi::Unknown;
#if SPARKLE_TARGET_EDITOR && !SPARKLE_BUILD_SHIPPING
	ExternalCaptureProvider CaptureProvider = ExternalCaptureProvider::None;
#endif
};
