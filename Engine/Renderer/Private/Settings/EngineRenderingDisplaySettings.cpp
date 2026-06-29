#include "PCH.h"

#include "Settings/EngineRenderingDisplaySettings.h"

#include "Core/Public/Strings/StringUtils.h"
#include "Frame/Presentation/OutputEncodingSettings.h"
#include "Frame/Presentation/ToneMappingCVars.h"
#include "Frame/Presentation/ToneMappingSettings.h"
#include "RHI/Public/CVars/RHICVars.h"
#include "RHI/Public/Presentation/RhiPresentationDefaults.h"

#include <string>

namespace
{
	const char* ToConfigString(EngineToneMapper toneMapper) noexcept
	{
		switch (SanitizeToneMapper(toneMapper))
		{
			case EngineToneMapper::Reinhard:
				return "Reinhard";
			case EngineToneMapper::AcesFilmic:
				return "AcesFilmic";
			case EngineToneMapper::AcesApprox:
			default:
				return "AcesApprox";
		}
	}

	const char* ToConfigString(EngineExposureMode mode) noexcept
	{
		return SanitizeExposureMode(mode) == EngineExposureMode::Manual ? "Manual" : "Automatic";
	}

	const char* ToConfigString(EngineExposureMeteringMethod method) noexcept
	{
		switch (SanitizeExposureMeteringMethod(method))
		{
			case EngineExposureMeteringMethod::DownsamplePyramid:
				return "DownsamplePyramid";
			case EngineExposureMeteringMethod::ParallelReduction:
			default:
				return "ParallelReduction";
		}
	}

	const char* ToConfigString(EngineOutputColorEncoding encoding) noexcept
	{
		switch (SanitizeOutputColorEncoding(encoding))
		{
			case EngineOutputColorEncoding::Linear:
				return "Linear";
			case EngineOutputColorEncoding::Srgb:
				return "Srgb";
			case EngineOutputColorEncoding::Automatic:
			default:
				return "Automatic";
		}
	}
}

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

EngineToneMapper EngineRenderingDisplaySettings::ParseToneMapper(std::string_view text) noexcept
{
	const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
	if (Strings::EqualsIgnoreCase(trimmed, "Reinhard"))
	{
		return EngineToneMapper::Reinhard;
	}
	if (Strings::EqualsIgnoreCase(trimmed, "AcesFilmic") || Strings::EqualsIgnoreCase(trimmed, "ACESFilmic") ||
	    Strings::EqualsIgnoreCase(trimmed, "ACES Filmic"))
	{
		return EngineToneMapper::AcesFilmic;
	}
	return EngineToneMapper::AcesApprox;
}

EngineExposureMode EngineRenderingDisplaySettings::ParseExposureMode(std::string_view text) noexcept
{
	const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
	return Strings::EqualsIgnoreCase(trimmed, "Manual") ? EngineExposureMode::Manual : EngineExposureMode::Automatic;
}

EngineExposureMeteringMethod EngineRenderingDisplaySettings::ParseExposureMeteringMethod(std::string_view text) noexcept
{
	const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
	if (Strings::EqualsIgnoreCase(trimmed, "DownsamplePyramid") ||
	    Strings::EqualsIgnoreCase(trimmed, "Downsample pyramid"))
	{
		return EngineExposureMeteringMethod::DownsamplePyramid;
	}
	return EngineExposureMeteringMethod::ParallelReduction;
}

EngineOutputColorEncoding EngineRenderingDisplaySettings::ParseOutputColorEncoding(std::string_view text) noexcept
{
	const std::string_view trimmed = Strings::TrimAsciiWhitespace(text);
	if (Strings::EqualsIgnoreCase(trimmed, "Linear"))
	{
		return EngineOutputColorEncoding::Linear;
	}
	if (Strings::EqualsIgnoreCase(trimmed, "Srgb") || Strings::EqualsIgnoreCase(trimmed, "sRGB"))
	{
		return EngineOutputColorEncoding::Srgb;
	}
	return EngineOutputColorEncoding::Automatic;
}

void EngineRenderingDisplaySettings::Capture(EngineRenderingSettingsState& state) noexcept
{
	state.VSync = CVarVSync.Get();
	state.BackBufferFormat = CVarBackBufferFormat.Get();
	state.PreferHighPerformanceAdapter = CVarPreferHighPerformanceAdapter.Get();
	state.ToneMapper = SanitizeToneMapper(CVarToneMapper.Get());
	state.ExposureMode = SanitizeExposureMode(CVarExposureMode.Get());
	state.ExposureMeteringMethod = SanitizeExposureMeteringMethod(CVarExposureMeteringMethod.Get());
	state.OutputColorEncoding = SanitizeOutputColorEncoding(CVarOutputColorEncoding.Get());
	state.ManualExposure = SanitizeManualExposure(CVarManualExposure.Get());
	state.ExposureCompensation = SanitizeExposureCompensation(CVarExposureCompensation.Get());
	state.ExposureTargetLuminance = SanitizeExposureTargetLuminance(CVarExposureTargetLuminance.Get());
	state.ExposureMin = SanitizeExposureMin(CVarExposureMin.Get());
	state.ExposureMax = SanitizeExposureMax(CVarExposureMax.Get());
	SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
	state.ExposureAdaptationSpeedUp = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedUp.Get());
	state.ExposureAdaptationSpeedDown = SanitizeExposureAdaptationSpeed(CVarExposureAdaptationSpeedDown.Get());
}

void EngineRenderingDisplaySettings::Apply(const EngineRenderingSettingsState& state) noexcept
{
	CVarVSync.Set(state.VSync);
	CVarBackBufferFormat.Set(state.BackBufferFormat);
	CVarPreferHighPerformanceAdapter.Set(state.PreferHighPerformanceAdapter);
	CVarToneMapper.Set(SanitizeToneMapper(state.ToneMapper));
	CVarExposureMode.Set(SanitizeExposureMode(state.ExposureMode));
	CVarExposureMeteringMethod.Set(SanitizeExposureMeteringMethod(state.ExposureMeteringMethod));
	CVarOutputColorEncoding.Set(SanitizeOutputColorEncoding(state.OutputColorEncoding));
	CVarManualExposure.Set(SanitizeManualExposure(state.ManualExposure));
	CVarExposureCompensation.Set(SanitizeExposureCompensation(state.ExposureCompensation));
	CVarExposureTargetLuminance.Set(SanitizeExposureTargetLuminance(state.ExposureTargetLuminance));
	float minExposure = state.ExposureMin;
	float maxExposure = state.ExposureMax;
	SanitizeExposureRange(minExposure, maxExposure);
	CVarExposureMin.Set(minExposure);
	CVarExposureMax.Set(maxExposure);
	CVarExposureAdaptationSpeedUp.Set(SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedUp));
	CVarExposureAdaptationSpeedDown.Set(SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedDown));
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
	if (trimmedKey == "ToneMapper")
	{
		state.ToneMapper = ParseToneMapper(trimmedValue);
		return true;
	}
	if (trimmedKey == "ExposureMode")
	{
		state.ExposureMode = ParseExposureMode(trimmedValue);
		return true;
	}
	if (trimmedKey == "ExposureMeteringMethod")
	{
		state.ExposureMeteringMethod = ParseExposureMeteringMethod(trimmedValue);
		return true;
	}
	if (trimmedKey == "OutputColorEncoding")
	{
		state.OutputColorEncoding = ParseOutputColorEncoding(trimmedValue);
		return true;
	}
	if (trimmedKey == "ManualExposure")
	{
		float exposure = state.ManualExposure;
		if (Strings::TryParseFloat(trimmedValue, exposure))
		{
			state.ManualExposure = SanitizeManualExposure(exposure);
		}
		return true;
	}
	if (trimmedKey == "ExposureCompensation")
	{
		float compensation = state.ExposureCompensation;
		if (Strings::TryParseFloat(trimmedValue, compensation))
		{
			state.ExposureCompensation = SanitizeExposureCompensation(compensation);
		}
		return true;
	}
	if (trimmedKey == "ExposureTargetLuminance")
	{
		float luminance = state.ExposureTargetLuminance;
		if (Strings::TryParseFloat(trimmedValue, luminance))
		{
			state.ExposureTargetLuminance = SanitizeExposureTargetLuminance(luminance);
		}
		return true;
	}
	if (trimmedKey == "ExposureMin")
	{
		float minExposure = state.ExposureMin;
		if (Strings::TryParseFloat(trimmedValue, minExposure))
		{
			state.ExposureMin = SanitizeExposureMin(minExposure);
			SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
		}
		return true;
	}
	if (trimmedKey == "ExposureMax")
	{
		float maxExposure = state.ExposureMax;
		if (Strings::TryParseFloat(trimmedValue, maxExposure))
		{
			state.ExposureMax = SanitizeExposureMax(maxExposure);
			SanitizeExposureRange(state.ExposureMin, state.ExposureMax);
		}
		return true;
	}
	if (trimmedKey == "ExposureAdaptationSpeedUp")
	{
		float speed = state.ExposureAdaptationSpeedUp;
		if (Strings::TryParseFloat(trimmedValue, speed))
		{
			state.ExposureAdaptationSpeedUp = SanitizeExposureAdaptationSpeed(speed);
		}
		return true;
	}
	if (trimmedKey == "ExposureAdaptationSpeedDown")
	{
		float speed = state.ExposureAdaptationSpeedDown;
		if (Strings::TryParseFloat(trimmedValue, speed))
		{
			state.ExposureAdaptationSpeedDown = SanitizeExposureAdaptationSpeed(speed);
		}
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
	values.emplace_back("ToneMapper", ToConfigString(state.ToneMapper));
	values.emplace_back("ExposureMode", ToConfigString(state.ExposureMode));
	values.emplace_back("ExposureMeteringMethod", ToConfigString(state.ExposureMeteringMethod));
	values.emplace_back("OutputColorEncoding", ToConfigString(state.OutputColorEncoding));
	values.emplace_back("ManualExposure", std::to_string(SanitizeManualExposure(state.ManualExposure)));
	values.emplace_back("ExposureCompensation", std::to_string(SanitizeExposureCompensation(state.ExposureCompensation)));
	values.emplace_back(
	    "ExposureTargetLuminance",
	    std::to_string(SanitizeExposureTargetLuminance(state.ExposureTargetLuminance)));
	float minExposure = state.ExposureMin;
	float maxExposure = state.ExposureMax;
	SanitizeExposureRange(minExposure, maxExposure);
	values.emplace_back("ExposureMin", std::to_string(minExposure));
	values.emplace_back("ExposureMax", std::to_string(maxExposure));
	values.emplace_back(
	    "ExposureAdaptationSpeedUp",
	    std::to_string(SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedUp)));
	values.emplace_back(
	    "ExposureAdaptationSpeedDown",
	    std::to_string(SanitizeExposureAdaptationSpeed(state.ExposureAdaptationSpeedDown)));
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
