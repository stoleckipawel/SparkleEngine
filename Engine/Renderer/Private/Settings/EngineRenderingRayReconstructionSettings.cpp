#include "../PCH.h"
#include "Settings/EngineRenderingRayReconstructionSettings.h"

#include "RayReconstruction/RayReconstructionSettings.h"

#include <charconv>
#include <system_error>

namespace
{
	template <typename TEnum>
	TEnum ParseEnumOrDefault(std::string_view value, TEnum fallback) noexcept
	{
		int parsed = 0;
		const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
		return result.ec == std::errc{} ? static_cast<TEnum>(parsed) : fallback;
	}
}

void EngineRenderingRayReconstructionSettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	const RayReconstructionSettings runtime = BuildRayReconstructionSettingsFromCVars();
	state.RayReconstructionMode = runtime.Mode;
	state.RayReconstructionQualityMode = runtime.QualityMode;
}

void EngineRenderingRayReconstructionSettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	SetRayReconstructionModeCVar(state.RayReconstructionMode);
	SetRayReconstructionQualityModeCVar(state.RayReconstructionQualityMode);
}

bool EngineRenderingRayReconstructionSettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	if (key == "RayReconstructionMode")
	{
		state.RayReconstructionMode = ParseEnumOrDefault(value, EngineRayReconstructionMode::Off);
		return true;
	}
	if (key == "RayReconstructionQualityMode")
	{
		state.RayReconstructionQualityMode = ParseEnumOrDefault(value, EngineRayReconstructionQualityMode::Quality);
		return true;
	}
	return false;
}

void EngineRenderingRayReconstructionSettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("RayReconstructionMode", std::to_string(static_cast<int>(state.RayReconstructionMode)));
	values.emplace_back("RayReconstructionQualityMode", std::to_string(static_cast<int>(state.RayReconstructionQualityMode)));
}
