#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include "Settings/EngineRenderingSettingsPersistence.h"
#include "Settings/EngineRenderingSettingsRuntime.h"

#include <sstream>
#include <utility>
#include <vector>

EngineRenderingSettingsSection::EngineRenderingSettingsSection()
{
	RefreshFromRuntimeState();
}

void EngineRenderingSettingsSection::RefreshFromRuntimeState() noexcept
{
	m_state = EngineRenderingSettingsRuntime::Capture();
	m_sessionBackBufferFormat = m_state.BackBufferFormat;
	m_sessionPreferHighPerformanceAdapter = m_state.PreferHighPerformanceAdapter;
}

void EngineRenderingSettingsSection::ApplyPersistedValuesToRuntimeState() noexcept
{
	EngineRenderingSettingsRuntime::ApplyPersistedValues();
	RefreshFromRuntimeState();
}

bool EngineRenderingSettingsSection::HasPendingRestart() const noexcept
{
	return ComputePendingRestart();
}

std::string EngineRenderingSettingsSection::BuildPendingRestartMessage() const
{
	return DescribePendingRestart();
}

void EngineRenderingSettingsSection::SetCommitHandler(CommitHandler handler)
{
	m_commitHandler = std::move(handler);
}

void EngineRenderingSettingsSection::CommitState()
{
	EngineRenderingSettingsPersistence::Write(m_state);
	if (m_commitHandler)
	{
		m_commitHandler(m_state);
		return;
	}
	ApplyEngineRenderingSettingsStateToCVars(m_state);
}

void EngineRenderingSettingsSection::SetVSync(bool enabled)
{
	SetValue(m_state.VSync, enabled);
}

void EngineRenderingSettingsSection::SetBackBufferFormat(PixelFormat format)
{
	SetValue(m_state.BackBufferFormat, format);
}

void EngineRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	SetValue(m_state.PreferHighPerformanceAdapter, enabled);
}

void EngineRenderingSettingsSection::SetToneMapper(EngineToneMapper toneMapper)
{
	SetValue(m_state.ToneMapper, toneMapper);
}

void EngineRenderingSettingsSection::SetExposureMode(EngineExposureMode mode)
{
	SetValue(m_state.ExposureMode, mode);
}

void EngineRenderingSettingsSection::SetExposureMeteringMethod(EngineExposureMeteringMethod method)
{
	SetValue(m_state.ExposureMeteringMethod, method);
}

void EngineRenderingSettingsSection::SetOutputColorEncoding(EngineOutputColorEncoding encoding)
{
	SetValue(m_state.OutputColorEncoding, encoding);
}

void EngineRenderingSettingsSection::SetManualExposure(float exposure)
{
	SetValue(m_state.ManualExposure, exposure);
}

void EngineRenderingSettingsSection::SetExposureCompensation(float compensation)
{
	SetValue(m_state.ExposureCompensation, compensation);
}

void EngineRenderingSettingsSection::SetExposureTargetLuminance(float luminance)
{
	SetValue(m_state.ExposureTargetLuminance, luminance);
}

void EngineRenderingSettingsSection::SetExposureMin(float exposure)
{
	SetValue(m_state.ExposureMin, exposure);
}

void EngineRenderingSettingsSection::SetExposureMax(float exposure)
{
	SetValue(m_state.ExposureMax, exposure);
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedUp(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedUp, speed);
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedDown(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedDown, speed);
}

void EngineRenderingSettingsSection::SetUpscalerProvider(EUpscalerProviderKind provider)
{
	SetValue(m_state.UpscalerProvider, provider);
}

void EngineRenderingSettingsSection::SetUpscalerQualityMode(EUpscalerQualityMode mode)
{
	SetValue(m_state.UpscalerQualityMode, mode);
}

void EngineRenderingSettingsSection::SetRayReconstructionMode(EngineRayReconstructionMode mode)
{
	SetValue(m_state.RayReconstructionMode, mode);
}

void EngineRenderingSettingsSection::SetGBufferMode(GBufferMode mode)
{
	SetValue(m_state.GBuffer, mode);
}

void EngineRenderingSettingsSection::SetLightingMode(LightingMode mode)
{
	SetValue(m_state.Lighting, mode);
}

void EngineRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	SetValue(m_state.MeshAutoBatching, enabled);
}

void EngineRenderingSettingsSection::SetRefitTlas(bool enabled)
{
	SetValue(m_state.RefitTlas, enabled);
}

void EngineRenderingSettingsSection::SetPtlasActive(bool active)
{
	SetValue(m_state.PtlasActive, active);
}

void EngineRenderingSettingsSection::SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis)
{
	SetValue(m_state.PtlasPartitionsPerAxis, partitionsPerAxis);
}

void EngineRenderingSettingsSection::SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode)
{
	SetValue(m_state.PtlasPartitionUpdateMode, mode);
}

void EngineRenderingSettingsSection::SetPtlasMarkAllDynamicInPartition(bool enabled)
{
	SetValue(m_state.PtlasMarkAllDynamicInPartition, enabled);
}

void EngineRenderingSettingsSection::SetPtlasModeChangeDistance(float distance)
{
	SetValue(m_state.PtlasModeChangeDistance, distance);
}

void EngineRenderingSettingsSection::SetRenderViewMode(RenderViewMode viewMode)
{
	SetValue(m_state.ViewMode, viewMode);
}

bool EngineRenderingSettingsSection::ComputePendingRestart() const noexcept
{
	return m_sessionPreferHighPerformanceAdapter != m_state.PreferHighPerformanceAdapter
	    || m_sessionBackBufferFormat != m_state.BackBufferFormat;
}

std::string EngineRenderingSettingsSection::DescribePendingRestart() const
{
	std::vector<std::string> reasons;
	if (m_sessionPreferHighPerformanceAdapter != m_state.PreferHighPerformanceAdapter)
	{
		reasons.emplace_back("GPU adapter preference");
	}
	if (m_sessionBackBufferFormat != m_state.BackBufferFormat)
	{
		reasons.emplace_back("back buffer format");
	}
	if (reasons.empty())
	{
		return {};
	}

	std::ostringstream stream;
	stream << "Restart the application to apply ";
	for (std::size_t index = 0; index < reasons.size(); ++index)
	{
		if (index > 0)
		{
			stream << (index + 1 == reasons.size() ? " and " : ", ");
		}
		stream << reasons[index];
	}
	stream << ".";
	return stream.str();
}

void ApplyEngineRenderingSettingsStateToCVars(const EngineRenderingSettingsState& state) noexcept
{
	EngineRenderingSettingsRuntime::Apply(state);
}

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsRuntime::ApplyPersistedValues();
}
