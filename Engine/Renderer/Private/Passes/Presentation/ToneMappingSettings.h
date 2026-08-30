#pragma once

#include "ShaderData/ExposureUniformData.h"
#include "ShaderData/ToneMappingUniformData.h"
#include "View/ViewportDisplaySettings.h"

ExposureUniformData BuildExposureUniformData(const ResolvedViewportDisplaySettings& settings, float frameDeltaSeconds) noexcept;
ToneMappingUniformData BuildToneMappingUniformData(EngineToneMapper toneMapper) noexcept;
