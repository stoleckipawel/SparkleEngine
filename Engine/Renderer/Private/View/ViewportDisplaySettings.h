#pragma once

#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

struct ResolvedViewportDisplaySettings final
{
	static ResolvedViewportDisplaySettings Resolve(const ViewportExposureOverrides& overrides) noexcept;

	EngineToneMapper ToneMapper = EngineToneMapper::AcesApprox;
	EngineExposureMode ExposureMode = EngineExposureMode::Automatic;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	float ManualExposure = 1.0f;
	float ExposureCompensation = 0.0f;
	float ExposureTargetLuminance = 0.18f;
	float ExposureMin = 0.000001f;
	float ExposureMax = 65536.0f;
	float ExposureAdaptationSpeedUp = 3.0f;
	float ExposureAdaptationSpeedDown = 1.0f;

private:
	static EngineExposureMode ResolveMode(EngineExposureMode requested, EngineExposureMode fallback) noexcept;
	static EngineExposureMeteringMethod ResolveMeteringMethod(
	    EngineExposureMeteringMethod requested,
	    EngineExposureMeteringMethod fallback) noexcept;
};
