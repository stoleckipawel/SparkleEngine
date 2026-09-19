#pragma once

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <functional>
#include <string>

class EngineRenderingSettingsSection final
{
public:
	using CommitHandler = std::function<void(EngineRenderingSettingsState)>;
	using RefreshHandler = std::function<EngineRenderingSettingsState()>;

	EngineRenderingSettingsSection(EngineRenderingSettingsState state, CommitHandler commitHandler, RefreshHandler refreshHandler);

	const EngineRenderingSettingsState& GetState() const noexcept { return m_state; }
	void RefreshFromRuntimeState() noexcept;
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
	void SetUpscalerProvider(EUpscalerProviderKind provider);
	void SetUpscalerQualityMode(EUpscalerQualityMode mode);
	void SetRayReconstructionMode(EngineRayReconstructionMode mode);
	void SetGBufferAlgorithm(GBufferAlgorithm algorithm);
	void SetMeshAutoBatching(bool enabled);
	void SetRefitTlas(bool enabled);
	void SetPtlasActive(bool active);
	void SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis);
	void SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode);
	void SetPtlasMarkAllDynamicInPartition(bool enabled);
	void SetPtlasModeChangeDistance(float distance);

private:
	template <typename TValue> void SetValue(TValue& destination, TValue value)
	{
		if (destination == value)
		{
			return;
		}
		destination = value;
		CommitState();
	}

	void CommitState();
	bool ComputePendingRestart() const noexcept;
	std::string DescribePendingRestart() const;

	EngineRenderingSettingsState m_state{};
	PixelFormat m_sessionBackBufferFormat = RhiPresentationDefaults::DefaultBackBufferFormat;
	bool m_sessionPreferHighPerformanceAdapter = true;
	CommitHandler m_commitHandler;
	RefreshHandler m_refreshHandler;
};
