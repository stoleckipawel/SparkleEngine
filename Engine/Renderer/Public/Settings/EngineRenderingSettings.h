#pragma once

#include "RHI/Public/Presentation/RhiPresentationDefaults.h"
#include "Renderer/Public/Debug/RenderViewMode.h"
#include "Renderer/Public/Settings/EngineRenderingDisplayTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"
#include "Renderer/Public/Settings/EngineRenderingRayTracingTypes.h"
#include "Renderer/Public/Settings/EngineRenderingUpscalingTypes.h"
#include "RendererAPI.h"

#include <cstdint>
#include <functional>
#include <string>

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
	std::uint32_t MaxDirectionalLights = 2;
	std::uint32_t MaxPointLights = 512;
	std::uint32_t MaxSpotLights = 512;
	std::uint32_t MaxRectLights = 128;
	EUpscalerProviderKind UpscalerProvider = EUpscalerProviderKind::Linear;
	EUpscalerQualityMode UpscalerQualityMode = EUpscalerQualityMode::NativeAA;
	EngineRayReconstructionMode RayReconstructionMode = EngineRayReconstructionMode::Off;
	GBufferMode GBuffer = GBufferMode::Rasterized;
	LightingMode Lighting = LightingMode::RestirPathTraced;
	bool MeshAutoBatching = true;
	bool RefitTlas = true;
	bool PtlasActive = false;
	std::uint32_t PtlasPartitionsPerAxis = 8;
	RayTracingPtlasPartitionUpdateMode PtlasPartitionUpdateMode = RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition;
	bool PtlasMarkAllDynamicInPartition = false;
	float PtlasModeChangeDistance = 100.0f;
	RenderViewMode ViewMode = RenderViewMode::Lit;
};

class SPARKLE_RENDERER_API EngineRenderingSettingsSection final
{
  public:
	using CommitHandler = std::function<void(EngineRenderingSettingsState)>;

	EngineRenderingSettingsSection();
	EngineRenderingSettingsSection(const EngineRenderingSettingsSection&) = delete;
	EngineRenderingSettingsSection& operator=(const EngineRenderingSettingsSection&) = delete;
	EngineRenderingSettingsSection(EngineRenderingSettingsSection&&) = delete;
	EngineRenderingSettingsSection& operator=(EngineRenderingSettingsSection&&) = delete;

	const EngineRenderingSettingsState& GetState() const noexcept { return m_state; }
	void SetCommitHandler(CommitHandler handler);
	void RefreshFromRuntimeState() noexcept;
	void ApplyPersistedValuesToRuntimeState() noexcept;
	bool HasPendingRestart() const noexcept;
	std::string BuildPendingRestartMessage() const;

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
	void SetUpscalerProvider(EUpscalerProviderKind provider);
	void SetUpscalerQualityMode(EUpscalerQualityMode mode);
	void SetRayReconstructionMode(EngineRayReconstructionMode mode);
	void SetGBufferMode(GBufferMode mode);
	void SetLightingMode(LightingMode mode);
	void SetMeshAutoBatching(bool enabled);
	void SetRefitTlas(bool enabled);
	void SetPtlasActive(bool active);
	void SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis);
	void SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode);
	void SetPtlasMarkAllDynamicInPartition(bool enabled);
	void SetPtlasModeChangeDistance(float distance);
	void SetRenderViewMode(RenderViewMode viewMode);

  private:
	template <typename TValue>
	void SetValue(TValue& destination, TValue value)
	{
		if (destination == value)
		{
			return;
		}
		destination = value;
		CommitState();
	}

	void CommitState();
	EngineRenderingSettingsState CaptureRuntimeState() const noexcept;
	bool ComputePendingRestart(const EngineRenderingSettingsState& baseline, const EngineRenderingSettingsState& current) const noexcept;
	std::string DescribePendingRestart(const EngineRenderingSettingsState& baseline, const EngineRenderingSettingsState& current) const;

	EngineRenderingSettingsState m_state{};
	EngineRenderingSettingsState m_sessionBaseline{};
	CommitHandler m_commitHandler;
};

SPARKLE_RENDERER_API void ApplyPersistedEngineRenderingSettingsToCVars() noexcept;
SPARKLE_RENDERER_API void ApplyEngineRenderingSettingsStateToCVars(
    const EngineRenderingSettingsState& state) noexcept;
