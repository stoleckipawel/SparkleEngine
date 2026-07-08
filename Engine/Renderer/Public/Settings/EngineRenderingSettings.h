#pragma once

#include "Core/Public/Config/ConfigBackedSettings.h"
#include "RHI/Public/Presentation/RhiPresentationDefaults.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Settings/EngineRenderingUpscalingTypes.h"
#include "RendererAPI.h"

#include <cstdint>
#include <string>

struct EngineRenderingSettingsState final
{
	bool VSync = true;
	PixelFormat BackBufferFormat = RhiPresentationDefaults::BackBufferFormat;
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
	std::uint32_t MaxDirectionalLights = 2;
	std::uint32_t MaxPointLights = 512;
	std::uint32_t MaxSpotLights = 512;
	std::uint32_t MaxRectLights = 128;
	EngineUpscalerProvider UpscalerProvider = EngineUpscalerProvider::NvidiaDlss;
	EngineUpscalerQualityMode UpscalerQualityMode = EngineUpscalerQualityMode::NativeAA;
	EngineRayReconstructionMode RayReconstructionMode = EngineRayReconstructionMode::Off;
	bool MeshAutoBatching = true;
	bool RefitTlas = true;
	bool PtlasActive = false;
	std::uint32_t PtlasPartitionsPerAxis = 8;
	EnginePtlasPartitionUpdateMode PtlasPartitionUpdateMode = EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool PtlasMarkAllDynamicInPartition = false;
	float PtlasModeChangeDistance = 100.0f;
	std::uint32_t IndirectDiffuseBounceCount = 1;
	std::uint32_t IndirectSpecularBounceCount = 1;
};

class SPARKLE_RENDERER_API EngineRenderingSettingsSection final : public ConfigBackedSettingsSection<EngineRenderingSettingsState>
{
  public:
	EngineRenderingSettingsSection();

	void SetVSync(bool enabled);
	void SetBackBufferFormat(PixelFormat format);
	void SetPreferHighPerformanceAdapter(bool enabled);
	void SetToneMapper(EngineToneMapper toneMapper);
	void SetExposureMode(EngineExposureMode mode);
	void SetExposureMeteringMethod(EngineExposureMeteringMethod method);
	void SetOutputColorEncoding(EngineOutputColorEncoding encoding);
	void SetManualExposure(float exposure);
	void SetExposureCompensation(float compensation);
	void SetExposureTargetLuminance(float luminance);
	void SetExposureMin(float exposure);
	void SetExposureMax(float exposure);
	void SetExposureAdaptationSpeedUp(float speed);
	void SetExposureAdaptationSpeedDown(float speed);
	void SetMaxDirectionalLights(std::uint32_t count);
	void SetMaxPointLights(std::uint32_t count);
	void SetMaxSpotLights(std::uint32_t count);
	void SetMaxRectLights(std::uint32_t count);
	void SetUpscalerProvider(EngineUpscalerProvider provider);
	void SetUpscalerQualityMode(EngineUpscalerQualityMode mode);
	void SetRayReconstructionMode(EngineRayReconstructionMode mode);
	void SetMeshAutoBatching(bool enabled);
	void SetRefitTlas(bool enabled);
	void SetPtlasActive(bool active);
	void SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis);
	void SetPtlasPartitionUpdateMode(EnginePtlasPartitionUpdateMode mode);
	void SetPtlasMarkAllDynamicInPartition(bool enabled);
	void SetPtlasModeChangeDistance(float distance);
	void SetIndirectDiffuseBounceCount(std::uint32_t bounceCount);
	void SetIndirectSpecularBounceCount(std::uint32_t bounceCount);

  private:
	EngineRenderingSettingsState CaptureRuntimeState() const noexcept override;
	void ApplyStateToRuntime(const EngineRenderingSettingsState& state) const noexcept override;
	void ReadConfigValue(EngineRenderingSettingsState& state, std::string_view key, std::string_view value) const override;
	std::vector<std::pair<std::string, std::string>> BuildConfigValues(const EngineRenderingSettingsState& state) const override;
	bool ComputePendingRestart(
	    const EngineRenderingSettingsState& baseline,
	    const EngineRenderingSettingsState& current) const noexcept override;
	std::string DescribePendingRestart(
	    const EngineRenderingSettingsState& baseline,
	    const EngineRenderingSettingsState& current) const override;
};

SPARKLE_RENDERER_API void ApplyPersistedEngineRenderingSettingsToCVars() noexcept;
