#pragma once

#include "Frame/PostProcessing/ExposureUniformData.h"
#include "Frame/Presentation/ToneMappingUniformData.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"

EngineToneMapper SanitizeToneMapper(EngineToneMapper toneMapper) noexcept;
EngineExposureMode SanitizeExposureMode(EngineExposureMode mode) noexcept;
EngineExposureMeteringMethod SanitizeExposureMeteringMethod(EngineExposureMeteringMethod method) noexcept;
float SanitizeManualExposure(float exposure) noexcept;
float SanitizeExposureCompensation(float compensation) noexcept;
float SanitizeExposureTargetLuminance(float luminance) noexcept;
float SanitizeExposureMin(float exposure) noexcept;
float SanitizeExposureMax(float exposure) noexcept;
float SanitizeExposureAdaptationSpeed(float speed) noexcept;
void SanitizeExposureRange(float& minExposure, float& maxExposure) noexcept;

ExposureUniformData BuildExposureUniformDataFromCVars(float frameDeltaSeconds, bool exposureHistoryValid) noexcept;
ToneMappingUniformData BuildToneMappingUniformDataFromCVars() noexcept;
