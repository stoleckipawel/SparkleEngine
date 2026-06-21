#include "PCH.h"

#include "Settings/EngineRenderingRayTracingSettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "Renderer/Public/Debug/RendererCVars.h"

#include <algorithm>

namespace
{
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
}

std::uint32_t EngineRenderingRayTracingSettings::SanitizePtlasPartitionsPerAxis(std::uint32_t partitionsPerAxis) noexcept
{
	return std::clamp(partitionsPerAxis, 1u, 64u);
}

float EngineRenderingRayTracingSettings::SanitizePtlasModeChangeDistance(float distance) noexcept
{
	return (std::max)(distance, 0.0f);
}

void EngineRenderingRayTracingSettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	state.RefitTlas = CVarRayTracingClassicTlasRefit.Get();
	state.PtlasActive = CVarRayTracingPreferPartitionedTlas.Get();
	state.PtlasPartitionsPerAxis = SanitizePtlasPartitionsPerAxis(CVarRayTracingPartitionsPerAxis.Get());
	state.PtlasPartitionTopology = FromRuntimePartitionTopology(CVarRayTracingPtlasPartitionTopology.Get());
	state.PtlasPartitionUpdateMode = FromRuntimePartitionUpdateMode(CVarRayTracingPtlasPartitionUpdateMode.Get());
	state.PtlasMarkAllDynamicInPartition = CVarRayTracingPtlasMarkAllDynamicInPartition.Get();
	state.PtlasModeChangeDistance = SanitizePtlasModeChangeDistance(CVarRayTracingPtlasModeChangeDistance.Get());
}

void EngineRenderingRayTracingSettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	CVarRayTracingClassicTlasRefit.Set(state.RefitTlas);
	CVarRayTracingPreferPartitionedTlas.Set(state.PtlasActive);
	CVarRayTracingPartitionsPerAxis.Set(SanitizePtlasPartitionsPerAxis(state.PtlasPartitionsPerAxis));
	CVarRayTracingPtlasPartitionTopology.Set(ToRuntimePartitionTopology(state.PtlasPartitionTopology));
	CVarRayTracingPtlasPartitionUpdateMode.Set(ToRuntimePartitionUpdateMode(state.PtlasPartitionUpdateMode));
	CVarRayTracingPtlasMarkAllDynamicInPartition.Set(state.PtlasMarkAllDynamicInPartition);
	CVarRayTracingPtlasModeChangeDistance.Set(SanitizePtlasModeChangeDistance(state.PtlasModeChangeDistance));
}

bool EngineRenderingRayTracingSettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	const std::string trimmedValue = Strings::TrimCopy(value);
	if (trimmedKey == "RefitTlas")
	{
		(void)Strings::TryParseBool(trimmedValue, state.RefitTlas);
		return true;
	}
	if (trimmedKey == "PtlasActive")
	{
		(void)Strings::TryParseBool(trimmedValue, state.PtlasActive);
		return true;
	}
	if (trimmedKey == "RayTracingTopLevelMode")
	{
		state.PtlasActive = Strings::EqualsIgnoreCase(Strings::TrimAsciiWhitespace(trimmedValue), "PartitionedTlas");
		return true;
	}
	if (trimmedKey == "PtlasPartitionsPerAxis")
	{
		std::uint32_t partitionsPerAxis = state.PtlasPartitionsPerAxis;
		if (Strings::TryParseNumber(trimmedValue, partitionsPerAxis))
		{
			state.PtlasPartitionsPerAxis = SanitizePtlasPartitionsPerAxis(partitionsPerAxis);
		}
		return true;
	}
	if (trimmedKey == "PtlasPartitionTopology")
	{
		state.PtlasPartitionTopology = ParsePtlasPartitionTopology(trimmedValue);
		return true;
	}
	if (trimmedKey == "PtlasPartitionUpdateMode")
	{
		state.PtlasPartitionUpdateMode = ParsePtlasPartitionUpdateMode(trimmedValue);
		return true;
	}
	if (trimmedKey == "PtlasMarkAllDynamicInPartition")
	{
		(void)Strings::TryParseBool(trimmedValue, state.PtlasMarkAllDynamicInPartition);
		return true;
	}
	if (trimmedKey == "PtlasModeChangeDistance")
	{
		float distance = state.PtlasModeChangeDistance;
		if (Strings::TryParseFloat(trimmedValue, distance))
		{
			state.PtlasModeChangeDistance = SanitizePtlasModeChangeDistance(distance);
		}
		return true;
	}
	return false;
}

void EngineRenderingRayTracingSettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("RefitTlas", state.RefitTlas ? "true" : "false");
	values.emplace_back("PtlasActive", state.PtlasActive ? "true" : "false");
	values.emplace_back("PtlasPartitionsPerAxis", std::to_string(SanitizePtlasPartitionsPerAxis(state.PtlasPartitionsPerAxis)));
	values.emplace_back("PtlasPartitionTopology", ToConfigString(state.PtlasPartitionTopology));
	values.emplace_back("PtlasPartitionUpdateMode", ToConfigString(state.PtlasPartitionUpdateMode));
	values.emplace_back("PtlasMarkAllDynamicInPartition", state.PtlasMarkAllDynamicInPartition ? "true" : "false");
	values.emplace_back("PtlasModeChangeDistance", std::to_string(SanitizePtlasModeChangeDistance(state.PtlasModeChangeDistance)));
}
