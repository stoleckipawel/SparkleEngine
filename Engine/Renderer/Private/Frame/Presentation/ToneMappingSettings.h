#pragma once

#include "Frame/PostProcessing/ExposureUniformData.h"
#include "Frame/Presentation/ToneMappingUniformData.h"
#include "Frame/Presentation/ViewportDisplaySettings.h"

ExposureUniformData BuildExposureUniformData(
    const ResolvedViewportDisplaySettings& settings,
    float frameDeltaSeconds,
    bool exposureHistoryValid) noexcept;
ToneMappingUniformData BuildToneMappingUniformData(EngineToneMapper toneMapper) noexcept;
