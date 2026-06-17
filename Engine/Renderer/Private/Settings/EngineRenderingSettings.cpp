#include "PCH.h"

#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

namespace
{
	constexpr std::string_view kRenderingSettingsSection = "/Script/SparkleRenderer.EngineRenderingSettings";

	EnginePtlasPartitionTopology ParsePtlasPartitionTopology(std::string_view text) noexcept
	{
		const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
		if (Strings::EqualsIgnoreCase(trimmed, "XZ2D") || Strings::EqualsIgnoreCase(trimmed, "2D_XZ") ||
		    Strings::EqualsIgnoreCase(trimmed, "2D X/Z"))
		{
			return EnginePtlasPartitionTopology::XZ2D;
		}
		return EnginePtlasPartitionTopology::XYZ3D;
	}

	EnginePtlasPartitionUpdateMode ParsePtlasPartitionUpdateMode(std::string_view text) noexcept
	{
		const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
		if (Strings::EqualsIgnoreCase(trimmed, "AlwaysMoveDynamicToGlobal") ||
		    Strings::EqualsIgnoreCase(trimmed, "MoveToGlobal") ||
		    Strings::EqualsIgnoreCase(trimmed, "Always move dynamic to global"))
		{
			return EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal;
		}
		if (Strings::EqualsIgnoreCase(trimmed, "UpdatePartitionNearbyMoveToGlobalOtherwise") ||
		    Strings::EqualsIgnoreCase(trimmed, "UpdateOrMoveToGlobal") ||
		    Strings::EqualsIgnoreCase(trimmed, "Update partition nearby, move to global otherwise"))
		{
			return EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
		}
		return EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition;
	}

	const char* ToConfigString(EnginePtlasPartitionTopology topology) noexcept
	{
		return topology == EnginePtlasPartitionTopology::XZ2D ? "XZ2D" : "XYZ3D";
	}

	const char* ToConfigString(EnginePtlasPartitionUpdateMode mode) noexcept
	{
		switch (mode)
		{
			case EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal:
				return "AlwaysMoveDynamicToGlobal";
			case EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise:
				return "UpdatePartitionNearbyMoveToGlobalOtherwise";
			case EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition:
			default:
				return "AlwaysUpdatePartition";
		}
	}

	RayTracingPtlasPartitionTopology ToRuntimePartitionTopology(EnginePtlasPartitionTopology topology) noexcept
	{
		return topology == EnginePtlasPartitionTopology::XZ2D ? RayTracingPtlasPartitionTopology::XZ2D
		                                                      : RayTracingPtlasPartitionTopology::XYZ3D;
	}

	EnginePtlasPartitionTopology FromRuntimePartitionTopology(RayTracingPtlasPartitionTopology topology) noexcept
	{
		return topology == RayTracingPtlasPartitionTopology::XZ2D ? EnginePtlasPartitionTopology::XZ2D
		                                                          : EnginePtlasPartitionTopology::XYZ3D;
	}

	RayTracingPtlasPartitionUpdateMode ToRuntimePartitionUpdateMode(EnginePtlasPartitionUpdateMode mode) noexcept
	{
		switch (mode)
		{
			case EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal:
				return RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal;
			case EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise:
				return RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
			case EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition:
			default:
				return RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition;
		}
	}

	EnginePtlasPartitionUpdateMode FromRuntimePartitionUpdateMode(RayTracingPtlasPartitionUpdateMode mode) noexcept
	{
		switch (mode)
		{
			case RayTracingPtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal:
				return EnginePtlasPartitionUpdateMode::AlwaysMoveDynamicToGlobal;
			case RayTracingPtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise:
				return EnginePtlasPartitionUpdateMode::UpdatePartitionNearbyMoveToGlobalOtherwise;
			case RayTracingPtlasPartitionUpdateMode::AlwaysUpdatePartition:
			default:
				return EnginePtlasPartitionUpdateMode::AlwaysUpdatePartition;
		}
	}

	std::uint32_t SanitizePtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis) noexcept
	{
		return std::clamp(partitionsPerAxis, 1u, 64u);
	}

	float SanitizePtlasModeChangeDistance(float distance) noexcept
	{
		return (std::max)(distance, 0.0f);
	}
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
	const std::uint32_t sanitizedPartitionsPerAxis = SanitizePtlasPartitionsPerAxis(partitionsPerAxis);
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
	const float sanitizedDistance = SanitizePtlasModeChangeDistance(distance);
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
	state.VSync = CVarRhiVSync.Get();
	state.PreferHighPerformanceAdapter = CVarRhiPreferHighPerformanceAdapter.Get();
	state.MeshAutoBatching = CVarRendererMeshAutoBatching.Get();
	state.RefitTlas = CVarRayTracingClassicTlasRefit.Get();
	state.PtlasActive = CVarRhiRayTracingPreferPartitionedTlas.Get();
	state.PtlasPartitionsPerAxis = SanitizePtlasPartitionsPerAxis(CVarRayTracingPartitionsPerAxis.Get());
	state.PtlasPartitionTopology = FromRuntimePartitionTopology(CVarRayTracingPtlasPartitionTopology.Get());
	state.PtlasPartitionUpdateMode = FromRuntimePartitionUpdateMode(CVarRayTracingPtlasPartitionUpdateMode.Get());
	state.PtlasMarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get();
	state.PtlasModeChangeDistance = SanitizePtlasModeChangeDistance(CVarRayTracingPtlasModeChangeDistance.Get());
	return state;
}

void EngineRenderingSettingsSection::ApplyStateToRuntime(const EngineRenderingSettingsState& state) const noexcept
{
	CVarRhiVSync.Set(state.VSync);
	CVarRhiPreferHighPerformanceAdapter.Set(state.PreferHighPerformanceAdapter);
	CVarRendererMeshAutoBatching.Set(state.MeshAutoBatching);
	CVarRayTracingClassicTlasRefit.Set(state.RefitTlas);
	CVarRhiRayTracingPreferPartitionedTlas.Set(state.PtlasActive);
	CVarRayTracingPartitionsPerAxis.Set(SanitizePtlasPartitionsPerAxis(state.PtlasPartitionsPerAxis));
	CVarRayTracingPtlasPartitionTopology.Set(ToRuntimePartitionTopology(state.PtlasPartitionTopology));
	CVarRayTracingPtlasPartitionUpdateMode.Set(ToRuntimePartitionUpdateMode(state.PtlasPartitionUpdateMode));
	CVarRayTracingPtlasMarkAllDynamicInPartition.Set(state.PtlasMarkAllDynamicInPartition);
	CVarRayTracingPtlasModeChangeDistance.Set(SanitizePtlasModeChangeDistance(state.PtlasModeChangeDistance));
}

void EngineRenderingSettingsSection::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value) const
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	const std::string trimmedValue = Strings::TrimCopy(value);
	if (trimmedKey == "VSync")
	{
		(void) Strings::TryParseBool(trimmedValue, state.VSync);
	}
	else if (trimmedKey == "PreferHighPerformanceAdapter")
	{
		(void) Strings::TryParseBool(trimmedValue, state.PreferHighPerformanceAdapter);
	}
	else if (trimmedKey == "MeshAutoBatching")
	{
		(void) Strings::TryParseBool(trimmedValue, state.MeshAutoBatching);
	}
	else if (trimmedKey == "RefitTlas")
	{
		(void) Strings::TryParseBool(trimmedValue, state.RefitTlas);
	}
	else if (trimmedKey == "PtlasActive")
	{
		(void) Strings::TryParseBool(trimmedValue, state.PtlasActive);
	}
	else if (trimmedKey == "RayTracingTopLevelMode")
	{
		state.PtlasActive = Strings::EqualsIgnoreCase(Strings::TrimAsciiWhitespace(trimmedValue), "PartitionedTlas");
	}
	else if (trimmedKey == "PtlasPartitionsPerAxis")
	{
		std::uint32_t partitionsPerAxis = state.PtlasPartitionsPerAxis;
		if (Strings::TryParseNumber(trimmedValue, partitionsPerAxis))
		{
			state.PtlasPartitionsPerAxis = SanitizePtlasPartitionsPerAxis(partitionsPerAxis);
		}
	}
	else if (trimmedKey == "PtlasPartitionTopology")
	{
		state.PtlasPartitionTopology = ParsePtlasPartitionTopology(trimmedValue);
	}
	else if (trimmedKey == "PtlasPartitionUpdateMode")
	{
		state.PtlasPartitionUpdateMode = ParsePtlasPartitionUpdateMode(trimmedValue);
	}
	else if (trimmedKey == "PtlasMarkAllDynamicInPartition")
	{
		(void) Strings::TryParseBool(trimmedValue, state.PtlasMarkAllDynamicInPartition);
	}
	else if (trimmedKey == "PtlasModeChangeDistance")
	{
		float distance = state.PtlasModeChangeDistance;
		if (Strings::TryParseFloat(trimmedValue, distance))
		{
			state.PtlasModeChangeDistance = SanitizePtlasModeChangeDistance(distance);
		}
	}
}

std::vector<std::pair<std::string, std::string>> EngineRenderingSettingsSection::BuildConfigValues(
    const EngineRenderingSettingsState& state) const
{
	return {
	    {"VSync", state.VSync ? "true" : "false"},
	    {"PreferHighPerformanceAdapter", state.PreferHighPerformanceAdapter ? "true" : "false"},
	    {"MeshAutoBatching", state.MeshAutoBatching ? "true" : "false"},
	    {"RefitTlas", state.RefitTlas ? "true" : "false"},
	    {"PtlasActive", state.PtlasActive ? "true" : "false"},
	    {"PtlasPartitionsPerAxis", std::to_string(SanitizePtlasPartitionsPerAxis(state.PtlasPartitionsPerAxis))},
	    {"PtlasPartitionTopology", ToConfigString(state.PtlasPartitionTopology)},
	    {"PtlasPartitionUpdateMode", ToConfigString(state.PtlasPartitionUpdateMode)},
	    {"PtlasMarkAllDynamicInPartition", state.PtlasMarkAllDynamicInPartition ? "true" : "false"},
	    {"PtlasModeChangeDistance", std::to_string(SanitizePtlasModeChangeDistance(state.PtlasModeChangeDistance))},
	};
}

bool EngineRenderingSettingsSection::ComputePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const noexcept
{
	return baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter;
}

std::string EngineRenderingSettingsSection::DescribePendingRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) const
{
	std::vector<std::string> reasons;
	if (baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter)
	{
		reasons.emplace_back("GPU adapter preference");
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

void ApplyPersistedEngineRenderingSettingsToCVars() noexcept
{
	EngineRenderingSettingsSection renderingSettings;
	renderingSettings.ApplyPersistedValuesToRuntimeState();
}
