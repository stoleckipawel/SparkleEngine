#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

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
	values.reserve(14);
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
