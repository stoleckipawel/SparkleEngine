#pragma once

#include "ShaderData/ExposureUniformData.h"
#include "View/ViewportDisplaySettings.h"

ExposureUniformData BuildExposureUniformData(const ResolvedViewportDisplaySettings& settings) noexcept;
