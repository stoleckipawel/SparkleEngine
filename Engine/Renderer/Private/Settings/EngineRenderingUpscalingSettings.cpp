#include "../PCH.h"
#include "Settings/EngineRenderingUpscalingSettings.h"

#include "Upscaling/UpscalerSettings.h"

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

	EngineUpscalerProvider ToEngineUpscalerProvider(EUpscalerProviderKind provider) noexcept
	{
		return provider == EUpscalerProviderKind::NvidiaDlss ? EngineUpscalerProvider::NvidiaDlss : EngineUpscalerProvider::Linear;
	}

	EUpscalerProviderKind ToRuntimeUpscalerProvider(EngineUpscalerProvider provider) noexcept
	{
		return provider == EngineUpscalerProvider::NvidiaDlss ? EUpscalerProviderKind::NvidiaDlss : EUpscalerProviderKind::Linear;
	}

	EngineUpscalerQualityMode ToEngineUpscalerQualityMode(EUpscalerQualityMode mode) noexcept
	{
		return static_cast<EngineUpscalerQualityMode>(mode);
	}

	EUpscalerQualityMode ToRuntimeUpscalerQualityMode(EngineUpscalerQualityMode mode) noexcept
	{
		return static_cast<EUpscalerQualityMode>(mode);
	}
}

void EngineRenderingUpscalingSettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	const UpscalerSettings runtime = BuildUpscalerSettingsFromCVars();
	state.UpscalerProvider = ToEngineUpscalerProvider(runtime.RequestedProvider);
	state.UpscalerQualityMode = ToEngineUpscalerQualityMode(runtime.QualityMode);
}

void EngineRenderingUpscalingSettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	SetUpscalerProviderCVar(ToRuntimeUpscalerProvider(state.UpscalerProvider));
	SetUpscalerQualityModeCVar(ToRuntimeUpscalerQualityMode(state.UpscalerQualityMode));
}

bool EngineRenderingUpscalingSettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	if (key == "UpscalerProvider")
	{
		state.UpscalerProvider = ParseEnumOrDefault(value, EngineUpscalerProvider::NvidiaDlss);
		return true;
	}
	if (key == "UpscalerQualityMode")
	{
		state.UpscalerQualityMode = ParseEnumOrDefault(value, EngineUpscalerQualityMode::NativeAA);
		return true;
	}
	return false;
}

void EngineRenderingUpscalingSettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("UpscalerProvider", std::to_string(static_cast<int>(state.UpscalerProvider)));
	values.emplace_back("UpscalerQualityMode", std::to_string(static_cast<int>(state.UpscalerQualityMode)));
}
