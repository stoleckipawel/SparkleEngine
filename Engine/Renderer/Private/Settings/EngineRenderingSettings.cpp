#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include "Frame/Presentation/OutputEncodingSettings.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "Settings/EngineRenderingDisplaySettings.h"
#include "Settings/EngineRenderingGeometrySettings.h"
#include "Settings/EngineRenderingLightingSettings.h"
#include "Settings/EngineRenderingRayTracingSettings.h"

#include <sstream>
#include <string>
#include <vector>

namespace
{
	constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleRenderer.EngineRenderingSettings";
}

EngineRenderingSettingsSection::EngineRenderingSettingsSection() :
	ConfigBackedSettingsSection(ConfigBackedSettings::DefaultProjectConfigPath("Engine"), kRenderingSettingsSection)
{
	RefreshFromRuntimeState();
}

void EngineRenderingSettingsSection::SetVSync(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.VSync == enabled)
	{
		return;
	}
	state.VSync = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetBackBufferFormat(PixelFormat format)
{
	EngineRenderingSettingsState state = GetState();
	if (state.BackBufferFormat == format)
	{
		return;
	}
	state.BackBufferFormat = format;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPreferHighPerformanceAdapter(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PreferHighPerformanceAdapter == enabled)
	{
		return;
	}
	state.PreferHighPerformanceAdapter = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetToneMapper(EngineToneMapper toneMapper)
{
	const EngineToneMapper sanitizedToneMapper = SanitizeToneMapper(toneMapper);
	EngineRenderingSettingsState state = GetState();
	if (state.ToneMapper == sanitizedToneMapper)
	{
		return;
	}
	state.ToneMapper = sanitizedToneMapper;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureMode(EngineExposureMode mode)
{
	const EngineExposureMode sanitizedMode = SanitizeExposureMode(mode);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureMode == sanitizedMode)
	{
		return;
	}
	state.ExposureMode = sanitizedMode;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureMeteringMethod(EngineExposureMeteringMethod method)
{
	const EngineExposureMeteringMethod sanitizedMethod = SanitizeExposureMeteringMethod(method);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureMeteringMethod == sanitizedMethod)
	{
		return;
	}
	state.ExposureMeteringMethod = sanitizedMethod;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetOutputColorEncoding(EngineOutputColorEncoding encoding)
{
	const EngineOutputColorEncoding sanitizedEncoding = SanitizeOutputColorEncoding(encoding);
	EngineRenderingSettingsState state = GetState();
	if (state.OutputColorEncoding == sanitizedEncoding)
	{
		return;
	}
	state.OutputColorEncoding = sanitizedEncoding;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetManualExposure(float exposure)
{
	const float sanitizedExposure = SanitizeManualExposure(exposure);
	EngineRenderingSettingsState state = GetState();
	if (state.ManualExposure == sanitizedExposure)
	{
		return;
	}
	state.ManualExposure = sanitizedExposure;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureCompensation(float compensation)
{
	const float sanitizedCompensation = SanitizeExposureCompensation(compensation);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureCompensation == sanitizedCompensation)
	{
		return;
	}
	state.ExposureCompensation = sanitizedCompensation;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureTargetLuminance(float luminance)
{
	const float sanitizedLuminance = SanitizeExposureTargetLuminance(luminance);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureTargetLuminance == sanitizedLuminance)
	{
		return;
	}
	state.ExposureTargetLuminance = sanitizedLuminance;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureMin(float exposure)
{
	EngineRenderingSettingsState state = GetState();
	state.ExposureMin = SanitizeExposureMin(exposure);
	SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureMax(float exposure)
{
	EngineRenderingSettingsState state = GetState();
	state.ExposureMax = SanitizeExposureMax(exposure);
	SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedUp(float speed)
{
	const float sanitizedSpeed = SanitizeExposureAdaptationSpeed(speed);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureAdaptationSpeedUp == sanitizedSpeed)
	{
		return;
	}
	state.ExposureAdaptationSpeedUp = sanitizedSpeed;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetExposureAdaptationSpeedDown(float speed)
{
	const float sanitizedSpeed = SanitizeExposureAdaptationSpeed(speed);
	EngineRenderingSettingsState state = GetState();
	if (state.ExposureAdaptationSpeedDown == sanitizedSpeed)
	{
		return;
	}
	state.ExposureAdaptationSpeedDown = sanitizedSpeed;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetMaxDirectionalLights(std::uint32_t count)
{
	EngineRenderingSettingsState state = GetState();
	if (state.MaxDirectionalLights == count)
	{
		return;
	}
	state.MaxDirectionalLights = count;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetMaxPointLights(std::uint32_t count)
{
	EngineRenderingSettingsState state = GetState();
	if (state.MaxPointLights == count)
	{
		return;
	}
	state.MaxPointLights = count;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetMaxSpotLights(std::uint32_t count)
{
	EngineRenderingSettingsState state = GetState();
	if (state.MaxSpotLights == count)
	{
		return;
	}
	state.MaxSpotLights = count;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetMeshAutoBatching(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.MeshAutoBatching == enabled)
	{
		return;
	}
	state.MeshAutoBatching = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetRefitTlas(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.RefitTlas == enabled)
	{
		return;
	}
	state.RefitTlas = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasActive(bool active)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasActive == active)
	{
		return;
	}
	state.PtlasActive = active;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis)
{
	const std::uint32_t sanitizedPartitionsPerAxis =
	    EngineRenderingRayTracingSettings::SanitizePtlasPartitionsPerAxis(partitionsPerAxis);
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasPartitionsPerAxis == sanitizedPartitionsPerAxis)
	{
		return;
	}
	state.PtlasPartitionsPerAxis = sanitizedPartitionsPerAxis;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasPartitionTopology(EnginePtlasPartitionTopology topology)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasPartitionTopology == topology)
	{
		return;
	}
	state.PtlasPartitionTopology = topology;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasPartitionUpdateMode(EnginePtlasPartitionUpdateMode mode)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasPartitionUpdateMode == mode)
	{
		return;
	}
	state.PtlasPartitionUpdateMode = mode;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasMarkAllDynamicInPartition(bool enabled)
{
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasMarkAllDynamicInPartition == enabled)
	{
		return;
	}
	state.PtlasMarkAllDynamicInPartition = enabled;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetPtlasModeChangeDistance(float distance)
{
	const float sanitizedDistance = EngineRenderingRayTracingSettings::SanitizePtlasModeChangeDistance(distance);
	EngineRenderingSettingsState state = GetState();
	if (state.PtlasModeChangeDistance == sanitizedDistance)
	{
		return;
	}
	state.PtlasModeChangeDistance = sanitizedDistance;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetIndirectDiffuseBounceCount(std::uint32_t bounceCount)
{
	const std::uint32_t sanitizedBounceCount = EngineRenderingRayTracingSettings::SanitizeIndirectBounceCount(bounceCount);
	EngineRenderingSettingsState state = GetState();
	if (state.IndirectDiffuseBounceCount == sanitizedBounceCount)
	{
		return;
	}
	state.IndirectDiffuseBounceCount = sanitizedBounceCount;
	UpdateState(state);
}

void EngineRenderingSettingsSection::SetIndirectSpecularBounceCount(std::uint32_t bounceCount)
{
	const std::uint32_t sanitizedBounceCount = EngineRenderingRayTracingSettings::SanitizeIndirectBounceCount(bounceCount);
	EngineRenderingSettingsState state = GetState();
	if (state.IndirectSpecularBounceCount == sanitizedBounceCount)
	{
		return;
	}
	state.IndirectSpecularBounceCount = sanitizedBounceCount;
	UpdateState(state);
}

EngineRenderingSettingsState EngineRenderingSettingsSection::CaptureRuntimeState() const noexcept
{
	EngineRenderingSettingsState state;
	EngineRenderingDisplaySettings::Capture(state);
	EngineRenderingLightingSettings::Capture(state);
	EngineRenderingGeometrySettings::Capture(state);
	EngineRenderingRayTracingSettings::Capture(state);
	return state;
}

void EngineRenderingSettingsSection::ApplyStateToRuntime(const EngineRenderingSettingsState& state) const noexcept
{
	EngineRenderingDisplaySettings::Apply(state);
	EngineRenderingLightingSettings::Apply(state);
	EngineRenderingGeometrySettings::Apply(state);
	EngineRenderingRayTracingSettings::Apply(state);
}

void EngineRenderingSettingsSection::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value) const
{
	if (EngineRenderingDisplaySettings::ReadConfigValue(state, key, value) ||
	    EngineRenderingLightingSettings::ReadConfigValue(state, key, value) ||
	    EngineRenderingGeometrySettings::ReadConfigValue(state, key, value) ||
	    EngineRenderingRayTracingSettings::ReadConfigValue(state, key, value))
	{
		return;
	}
}

std::vector<std::pair<std::string, std::string>> EngineRenderingSettingsSection::BuildConfigValues(
    const EngineRenderingSettingsState& state) const
{
	std::vector<std::pair<std::string, std::string>> values;
	values.reserve(24);
	EngineRenderingDisplaySettings::AppendConfigValues(state, values);
	EngineRenderingLightingSettings::AppendConfigValues(state, values);
	EngineRenderingGeometrySettings::AppendConfigValues(state, values);
	EngineRenderingRayTracingSettings::AppendConfigValues(state, values);
	return values;
}

bool EngineRenderingSettingsSection::ComputePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const noexcept
{
	return EngineRenderingDisplaySettings::RequiresRestart(baseline, current);
}

std::string EngineRenderingSettingsSection::DescribePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const
{
	std::vector<std::string> reasons;
	EngineRenderingDisplaySettings::AppendRestartReasons(baseline, current, reasons);
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

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
