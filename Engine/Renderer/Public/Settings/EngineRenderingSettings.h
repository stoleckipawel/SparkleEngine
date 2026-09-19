#pragma once

#include "RHI/Public/Presentation/RhiPresentationDefaults.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Settings/EngineRenderingUpscalingTypes.h"
#include "RendererAPI.h"

#include <cstdint>

struct EngineRenderingSettingsState final
{
	bool VSync = true;
	PixelFormat BackBufferFormat = RhiPresentationDefaults::DefaultBackBufferFormat;
	bool PreferHighPerformanceAdapter = true;
	EngineToneMapper ToneMapper = EngineToneMapper::AcesApprox;
	EngineExposureMode ExposureMode = EngineExposureMode::Automatic;
	EngineExposureMeteringMethod ExposureMeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	EngineOutputColorEncoding OutputColorEncoding = EngineOutputColorEncoding::Automatic;
	float ManualExposure = 1.0f;
	float ExposureCompensation = 0.0f;
	float ExposureTargetLuminance = 0.18f;
	float ExposureMin = 0.000001f;
	float ExposureMax = 65536.0f;
	float ExposureAdaptationSpeedUp = 3.0f;
	float ExposureAdaptationSpeedDown = 1.0f;
	EUpscalerProviderKind UpscalerProvider = EUpscalerProviderKind::Linear;
	EUpscalerQualityMode UpscalerQualityMode = EUpscalerQualityMode::NativeAA;
	EngineRayReconstructionMode RayReconstructionMode = EngineRayReconstructionMode::Off;
	GBufferAlgorithm SelectedGBufferAlgorithm = GBufferAlgorithm::Rasterized;
	bool MeshAutoBatching = true;
	bool RefitTlas = true;
	bool PtlasActive = false;
	std::uint32_t PtlasPartitionsPerAxis = 8;
	RayTracingPtlasPartitionUpdateMode PtlasPartitionUpdateMode = RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool PtlasMarkAllDynamicInPartition = false;
	float PtlasModeChangeDistance = 100.0f;
};
