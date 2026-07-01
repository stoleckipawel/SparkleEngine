#pragma once

#include "Upscaling/UpscalerInputContract.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

void FillStreamlineConstants(sl::Constants& constants, const UpscalerInputContract& inputContract) noexcept;
#endif
