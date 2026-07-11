#pragma once

#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "Upscaling/UpscalerSettings.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>
#include <sl_dlss.h>

sl::DLSSMode ToStreamlineDlssMode(EUpscalerQualityMode mode) noexcept;
sl::DLSSOptions BuildStreamlineDlssOptions(
    EUpscalerQualityMode qualityMode,
    RenderViewportExtent outputExtent) noexcept;
#endif
