#include "PCH.h"

#include "Settings/EngineRenderingSettingsController.h"

#include <sstream>
#include <utility>
#include <vector>

EngineRenderingSettingsController::EngineRenderingSettingsController(
    EngineRenderingSettingsState state,
    CommitHandler commitHandler,
    RefreshHandler refreshHandler) :
    m_state(std::move(state)),
    m_sessionBackBufferFormat(m_state.BackBufferFormat),
    m_sessionPreferHighPerformanceAdapter(m_state.PreferHighPerformanceAdapter),
    m_commitHandler(std::move(commitHandler)),
    m_refreshHandler(std::move(refreshHandler))
{
}

void EngineRenderingSettingsController::RefreshFromRuntimeState() noexcept
{
	if (!m_refreshHandler)
	{
		return;
	}
	m_state = m_refreshHandler();
	m_sessionBackBufferFormat = m_state.BackBufferFormat;
	m_sessionPreferHighPerformanceAdapter = m_state.PreferHighPerformanceAdapter;
}

bool EngineRenderingSettingsController::HasPendingRestart() const noexcept
{
	return ComputePendingRestart();
}

std::string EngineRenderingSettingsController::BuildPendingRestartMessage() const
{
	return DescribePendingRestart();
}

void EngineRenderingSettingsController::CommitState()
{
	if (m_commitHandler)
	{
		m_commitHandler(m_state);
	}
}

void EngineRenderingSettingsController::SetVSync(bool enabled)
{
	SetValue(m_state.VSync, enabled);
}

void EngineRenderingSettingsController::SetBackBufferFormat(PixelFormat format)
{
	SetValue(m_state.BackBufferFormat, format);
}

void EngineRenderingSettingsController::SetPreferHighPerformanceAdapter(bool enabled)
{
	SetValue(m_state.PreferHighPerformanceAdapter, enabled);
}

void EngineRenderingSettingsController::SetToneMapper(EngineToneMapper toneMapper)
{
	SetValue(m_state.ToneMapper, toneMapper);
}

void EngineRenderingSettingsController::SetExposureMode(EngineExposureMode mode)
{
	SetValue(m_state.ExposureMode, mode);
}

void EngineRenderingSettingsController::SetExposureMeteringMethod(EngineExposureMeteringMethod method)
{
	SetValue(m_state.ExposureMeteringMethod, method);
}

void EngineRenderingSettingsController::SetOutputColorEncoding(EngineOutputColorEncoding encoding)
{
	SetValue(m_state.OutputColorEncoding, encoding);
}

void EngineRenderingSettingsController::SetManualExposure(float exposure)
{
	SetValue(m_state.ManualExposure, exposure);
}

void EngineRenderingSettingsController::SetExposureCompensation(float compensation)
{
	SetValue(m_state.ExposureCompensation, compensation);
}

void EngineRenderingSettingsController::SetExposureTargetLuminance(float luminance)
{
	SetValue(m_state.ExposureTargetLuminance, luminance);
}

void EngineRenderingSettingsController::SetExposureMin(float exposure)
{
	SetValue(m_state.ExposureMin, exposure);
}

void EngineRenderingSettingsController::SetExposureMax(float exposure)
{
	SetValue(m_state.ExposureMax, exposure);
}

void EngineRenderingSettingsController::SetExposureAdaptationSpeedUp(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedUp, speed);
}

void EngineRenderingSettingsController::SetExposureAdaptationSpeedDown(float speed)
{
	SetValue(m_state.ExposureAdaptationSpeedDown, speed);
}

void EngineRenderingSettingsController::SetUpscalerProvider(EUpscalerProviderKind provider)
{
	SetValue(m_state.UpscalerProvider, provider);
}

void EngineRenderingSettingsController::SetUpscalerQualityMode(EUpscalerQualityMode mode)
{
	SetValue(m_state.UpscalerQualityMode, mode);
}

void EngineRenderingSettingsController::SetRayReconstructionMode(EngineRayReconstructionMode mode)
{
	SetValue(m_state.RayReconstructionMode, mode);
}

void EngineRenderingSettingsController::SetGBufferAlgorithm(GBufferAlgorithm algorithm)
{
	SetValue(m_state.SelectedGBufferAlgorithm, algorithm);
}

void EngineRenderingSettingsController::SetMeshAutoBatching(bool enabled)
{
	SetValue(m_state.MeshAutoBatching, enabled);
}

void EngineRenderingSettingsController::SetRefitTlas(bool enabled)
{
	SetValue(m_state.RefitTlas, enabled);
}

void EngineRenderingSettingsController::SetPtlasActive(bool active)
{
	SetValue(m_state.PtlasActive, active);
}

void EngineRenderingSettingsController::SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis)
{
	SetValue(m_state.PtlasPartitionsPerAxis, partitionsPerAxis);
}

void EngineRenderingSettingsController::SetPtlasPartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode)
{
	SetValue(m_state.PtlasPartitionUpdateMode, mode);
}

void EngineRenderingSettingsController::SetPtlasMarkAllDynamicInPartition(bool enabled)
{
	SetValue(m_state.PtlasMarkAllDynamicInPartition, enabled);
}

void EngineRenderingSettingsController::SetPtlasModeChangeDistance(float distance)
{
	SetValue(m_state.PtlasModeChangeDistance, distance);
}

bool EngineRenderingSettingsController::ComputePendingRestart() const noexcept
{
	return m_sessionPreferHighPerformanceAdapter != m_state.PreferHighPerformanceAdapter
	    || m_sessionBackBufferFormat != m_state.BackBufferFormat;
}

std::string EngineRenderingSettingsController::DescribePendingRestart() const
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
