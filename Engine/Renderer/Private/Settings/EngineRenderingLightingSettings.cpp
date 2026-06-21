#include "PCH.h"

#include "Settings/EngineRenderingLightingSettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Lighting/LightingCVars.h"

void EngineRenderingLightingSettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	state.MaxDirectionalLights = CVarMaxDirectionalLights.Get();
	state.MaxPointLights = CVarMaxPointLights.Get();
	state.MaxSpotLights = CVarMaxSpotLights.Get();
}

void EngineRenderingLightingSettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	CVarMaxDirectionalLights.Set(state.MaxDirectionalLights);
	CVarMaxPointLights.Set(state.MaxPointLights);
	CVarMaxSpotLights.Set(state.MaxSpotLights);
}

bool EngineRenderingLightingSettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	const std::string trimmedValue = Strings::TrimCopy(value);
	std::uint32_t count = 0;
	if (trimmedKey == "MaxDirectionalLights" && Strings::TryParseNumber(trimmedValue, count))
	{
		state.MaxDirectionalLights = count;
		return true;
	}
	if (trimmedKey == "MaxPointLights" && Strings::TryParseNumber(trimmedValue, count))
	{
		state.MaxPointLights = count;
		return true;
	}
	if (trimmedKey == "MaxSpotLights" && Strings::TryParseNumber(trimmedValue, count))
	{
		state.MaxSpotLights = count;
		return true;
	}
	return trimmedKey == "MaxDirectionalLights" || trimmedKey == "MaxPointLights" || trimmedKey == "MaxSpotLights";
}

void EngineRenderingLightingSettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("MaxDirectionalLights", std::to_string(state.MaxDirectionalLights));
	values.emplace_back("MaxPointLights", std::to_string(state.MaxPointLights));
	values.emplace_back("MaxSpotLights", std::to_string(state.MaxSpotLights));
}
