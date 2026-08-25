#include "PCH.h"
#include "Settings/EngineRenderingSettingsRuntime.h"

#include "Core/Public/Console/CVar.h"
#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Passes/Presentation/OutputEncodingCVars.h"
#include "View/ViewportDisplayCVars.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Renderer/Public/Debug/RendererCVars.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Settings/EngineRenderingSettingsPersistence.h"
#include "Upscaling/UpscalerSettings.h"

#include <string>
#include <string_view>

void EngineRenderingSettingsRuntime::ApplyPersistedValue(std::string_view key, std::string_view value)
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	if (!EngineRenderingSettingsPersistence::IsPersistedName(trimmedKey))
	{
		return;
	}

	ConsoleVariableBase* variable = ConsoleVariableRegistry::Get().Find(trimmedKey);
	if (variable == nullptr)
	{
		return;
	}

	const std::string persistedValue = Strings::TrimCopy(value);
	std::string errorMessage;
	(void) variable->TrySetValueFromString(persistedValue, errorMessage);
}

EngineRenderingSettingsState EngineRenderingSettingsRuntime::Capture() noexcept
{
	EngineRenderingSettingsState state;
	state.VSync = CVarVSync.Get();
	state.BackBufferFormat = CVarBackBufferFormat.Get();
	state.PreferHighPerformanceAdapter = CVarPreferHighPerformanceAdapter.Get();
	state.ToneMapper = CVarToneMapper.Get();
	state.ExposureMode = CVarExposureMode.Get();
	state.ExposureMeteringMethod = CVarExposureMeteringMethod.Get();
	state.OutputColorEncoding = CVarOutputColorEncoding.Get();
	state.ManualExposure = CVarManualExposure.Get();
	state.ExposureCompensation = CVarExposureCompensation.Get();
	state.ExposureTargetLuminance = CVarExposureTargetLuminance.Get();
	state.ExposureMin = CVarExposureMin.Get();
	state.ExposureMax = CVarExposureMax.Get();
	state.ExposureAdaptationSpeedUp = CVarExposureAdaptationSpeedUp.Get();
	state.ExposureAdaptationSpeedDown = CVarExposureAdaptationSpeedDown.Get();
	state.UpscalerProvider = CVarUpscalerProvider.Get();
	state.UpscalerQualityMode = CVarUpscalerQualityMode.Get();
	state.RayReconstructionMode = CVarRayReconstructionMode.Get();
	state.SelectedGBufferAlgorithm = CVarGBufferAlgorithm.Get();
	state.GBufferRayTracingExecutionMode = CVarGBufferRayTracingExecutionMode.Get();
	state.Lighting = CVarLightingMode.Get();
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
	state.RefitTlas = CVarRayTracingClassicTlasRefit.Get();
	state.PtlasActive = CVarRayTracingPreferPartitionedTlas.Get();
	state.PtlasPartitionsPerAxis = CVarRayTracingPartitionsPerAxis.Get();
	state.PtlasPartitionUpdateMode = CVarRayTracingPtlasPartitionUpdateMode.Get();
	state.PtlasMarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get();
	state.PtlasModeChangeDistance = CVarRayTracingPtlasModeChangeDistance.Get();
	state.ViewMode = CVarRenderViewMode.Get();
	return state;
}

void EngineRenderingSettingsRuntime::Apply(const EngineRenderingSettingsState& state) noexcept
{
	const auto setCVarIfChanged = []<typename TCVar, typename TValue>(TCVar& cvar, const TValue& value) noexcept
	{
		if (cvar.Get() != value)
		{
			cvar.Set(value);
		}
	};
	setCVarIfChanged(CVarVSync, state.VSync);
	setCVarIfChanged(CVarBackBufferFormat, state.BackBufferFormat);
	setCVarIfChanged(CVarPreferHighPerformanceAdapter, state.PreferHighPerformanceAdapter);
	setCVarIfChanged(CVarToneMapper, state.ToneMapper);
	setCVarIfChanged(CVarExposureMode, state.ExposureMode);
	setCVarIfChanged(CVarExposureMeteringMethod, state.ExposureMeteringMethod);
	setCVarIfChanged(CVarOutputColorEncoding, state.OutputColorEncoding);
	setCVarIfChanged(CVarManualExposure, state.ManualExposure);
	setCVarIfChanged(CVarExposureCompensation, state.ExposureCompensation);
	setCVarIfChanged(CVarExposureTargetLuminance, state.ExposureTargetLuminance);
	setCVarIfChanged(CVarExposureMin, state.ExposureMin);
	setCVarIfChanged(CVarExposureMax, state.ExposureMax);
	setCVarIfChanged(CVarExposureAdaptationSpeedUp, state.ExposureAdaptationSpeedUp);
	setCVarIfChanged(CVarExposureAdaptationSpeedDown, state.ExposureAdaptationSpeedDown);
	setCVarIfChanged(CVarUpscalerProvider, state.UpscalerProvider);
	setCVarIfChanged(CVarUpscalerQualityMode, state.UpscalerQualityMode);
	setCVarIfChanged(CVarRayReconstructionMode, state.RayReconstructionMode);
	setCVarIfChanged(CVarGBufferAlgorithm, state.SelectedGBufferAlgorithm);
	setCVarIfChanged(CVarGBufferRayTracingExecutionMode, state.GBufferRayTracingExecutionMode);
	setCVarIfChanged(CVarLightingMode, state.Lighting);
	setCVarIfChanged(CVarRendererMeshAutoBatching, state.MeshAutoBatching);
	setCVarIfChanged(CVarRayTracingClassicTlasRefit, state.RefitTlas);
	setCVarIfChanged(CVarRayTracingPreferPartitionedTlas, state.PtlasActive);
	setCVarIfChanged(CVarRayTracingPartitionsPerAxis, state.PtlasPartitionsPerAxis);
	setCVarIfChanged(CVarRayTracingPtlasPartitionUpdateMode, state.PtlasPartitionUpdateMode);
	setCVarIfChanged(CVarRayTracingPtlasMarkAllDynamicInPartition, state.PtlasMarkAllDynamicInPartition);
	setCVarIfChanged(CVarRayTracingPtlasModeChangeDistance, state.PtlasModeChangeDistance);
	setCVarIfChanged(CVarRenderViewMode, state.ViewMode);
}

void EngineRenderingSettingsRuntime::ApplyPersistedValues() noexcept
{
	EngineRenderingSettingsPersistence::Load(&EngineRenderingSettingsRuntime::ApplyPersistedValue);
}
