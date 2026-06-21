#include "PCH.h"

#include "Settings/EngineRenderingDisplaySettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RHI/Public/Presentation/RhiPresentationDefaults.h"

PixelFormat EngineRenderingDisplaySettings::ParseBackBufferFormat(std::string_view text) noexcept
{
	const std::string trimmed = Strings::TrimCopy(text);
	std::uint32_t numericValue = 0;
	if (Strings::TryParseNumber(trimmed, numericValue))
	{
		return PixelFormatFromSerializedTextureFormat(numericValue);
	}

	for (const PixelFormat format : RhiPresentationDefaults::SupportedBackBufferFormats)
	{
		if (Strings::EqualsIgnoreCase(trimmed, PixelFormatName(format)))
		{
			return format;
		}
	}

	return PixelFormat::Unknown;
}

void EngineRenderingDisplaySettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	state.VSync = CVarVSync.Get();
	state.BackBufferFormat = CVarBackBufferFormat.Get();
	state.PreferHighPerformanceAdapter = CVarPreferHighPerformanceAdapter.Get();
}

void EngineRenderingDisplaySettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	CVarVSync.Set(state.VSync);
	CVarBackBufferFormat.Set(state.BackBufferFormat);
	CVarPreferHighPerformanceAdapter.Set(state.PreferHighPerformanceAdapter);
}

bool EngineRenderingDisplaySettings::ReadConfigValue(
    EngineRenderingSettingsState& state,
    std::string_view key,
    std::string_view value)
{
	const std::string trimmedKey = Strings::TrimCopy(key);
	const std::string trimmedValue = Strings::TrimCopy(value);
	if (trimmedKey == "VSync")
	{
		(void)Strings::TryParseBool(trimmedValue, state.VSync);
		return true;
	}
	if (trimmedKey == "BackBufferFormat")
	{
		state.BackBufferFormat = ParseBackBufferFormat(trimmedValue);
		return true;
	}
	if (trimmedKey == "PreferHighPerformanceAdapter")
	{
		(void)Strings::TryParseBool(trimmedValue, state.PreferHighPerformanceAdapter);
		return true;
	}
	return false;
}

void EngineRenderingDisplaySettings::AppendConfigValues(
    const EngineRenderingSettingsState& state,
    std::vector<std::pair<std::string, std::string>>& values)
{
	values.emplace_back("VSync", state.VSync ? "true" : "false");
	values.emplace_back("BackBufferFormat", PixelFormatName(state.BackBufferFormat));
	values.emplace_back("PreferHighPerformanceAdapter", state.PreferHighPerformanceAdapter ? "true" : "false");
}

bool EngineRenderingDisplaySettings::RequiresRestart(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current) noexcept
{
	return baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter ||
	       baseline.BackBufferFormat != current.BackBufferFormat;
}

void EngineRenderingDisplaySettings::AppendRestartReasons(
    const EngineRenderingSettingsState& baseline,
    const EngineRenderingSettingsState& current,
    std::vector<std::string>& reasons)
{
	if (baseline.PreferHighPerformanceAdapter != current.PreferHighPerformanceAdapter)
	{
		reasons.emplace_back("GPU adapter preference");
	}
	if (baseline.BackBufferFormat != current.BackBufferFormat)
	{
		reasons.emplace_back("back buffer format");
	}
}
