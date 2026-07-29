#pragma once

#include "Frame/PostProcessing/ExposureUniformData.h"
#include "Frame/Presentation/ToneMappingUniformData.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"

ExposureUniformData BuildExposureUniformData(float frameDeltaSeconds, bool exposureHistoryValid) noexcept;
ToneMappingUniformData BuildToneMappingUniformData() noexcept;
