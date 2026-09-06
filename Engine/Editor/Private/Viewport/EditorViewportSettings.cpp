#include "PCH.h"

#include "Viewport/EditorViewportSettings.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

class EditorViewportSettingsSerialization final
{
public:
	template <typename TValue> static bool ParseNumber(std::string_view text, TValue& value) noexcept
	{
		const std::string trimmed = Strings::TrimCopy(text);
		const char* begin = trimmed.data();
		const char* end = begin + trimmed.size();
		const auto result = std::from_chars(begin, end, value);
		return result.ec == std::errc{} && result.ptr == end;
	}

	static bool ParseBool(std::string_view text, bool& value) noexcept
	{
		int parsed = 0;
		if (!ParseNumber(text, parsed) || (parsed != 0 && parsed != 1))
		{
			return false;
		}
		value = parsed != 0;
		return true;
	}

	template <typename TEnum> static bool ParseEnum(std::string_view text, TEnum& value) noexcept
	{
		int parsed = 0;
		if (!ParseNumber(text, parsed))
		{
			return false;
		}
		value = static_cast<TEnum>(parsed);
		return true;
	}

	static void Apply(std::string_view key, std::string_view value, EditorViewportSettingsState& state) noexcept
	{
		if (key == "MoveSpeedMetersPerSecond")
			(void) ParseNumber(value, state.Navigation.MoveSpeedMetersPerSecond);
		else if (key == "RotationSpeedDegreesPerPixel")
			(void) ParseNumber(value, state.Navigation.RotationSpeedDegreesPerPixel);
		else if (key == "InvertY")
			(void) ParseBool(value, state.Navigation.InvertY);
		else if (key == "ProjectionKind")
			(void) ParseEnum(value, state.ProjectionKind);
		else if (key == "OrthographicHeightMeters")
			(void) ParseNumber(value, state.OrthographicHeightMeters);
		else if (key == "OverrideExposureMode")
			(void) ParseBool(value, state.Exposure.OverrideMode);
		else if (key == "ExposureMode")
			(void) ParseEnum(value, state.Exposure.Mode);
		else if (key == "OverrideExposureMeteringMethod")
			(void) ParseBool(value, state.Exposure.OverrideMeteringMethod);
		else if (key == "ExposureMeteringMethod")
			(void) ParseEnum(value, state.Exposure.MeteringMethod);
		else if (key == "OverrideManualExposure")
			(void) ParseBool(value, state.Exposure.OverrideManualExposure);
		else if (key == "ManualExposure")
			(void) ParseNumber(value, state.Exposure.ManualExposure);
		else if (key == "OverrideExposureCompensation")
			(void) ParseBool(value, state.Exposure.OverrideCompensation);
		else if (key == "ExposureCompensation")
			(void) ParseNumber(value, state.Exposure.Compensation);
		else if (key == "OverrideExposureTargetLuminance")
			(void) ParseBool(value, state.Exposure.OverrideTargetLuminance);
		else if (key == "ExposureTargetLuminance")
			(void) ParseNumber(value, state.Exposure.TargetLuminance);
		else if (key == "OverrideExposureMinimum")
			(void) ParseBool(value, state.Exposure.OverrideMinimum);
		else if (key == "ExposureMinimum")
			(void) ParseNumber(value, state.Exposure.Minimum);
		else if (key == "OverrideExposureMaximum")
			(void) ParseBool(value, state.Exposure.OverrideMaximum);
		else if (key == "ExposureMaximum")
			(void) ParseNumber(value, state.Exposure.Maximum);
		else if (key == "OverrideExposureAdaptationSpeedUp")
			(void) ParseBool(value, state.Exposure.OverrideAdaptationSpeedUp);
		else if (key == "ExposureAdaptationSpeedUp")
			(void) ParseNumber(value, state.Exposure.AdaptationSpeedUp);
		else if (key == "OverrideExposureAdaptationSpeedDown")
			(void) ParseBool(value, state.Exposure.OverrideAdaptationSpeedDown);
		else if (key == "ExposureAdaptationSpeedDown")
			(void) ParseNumber(value, state.Exposure.AdaptationSpeedDown);
	}

	static void WriteBool(std::ofstream& output, std::string_view key, bool value) { output << key << '=' << (value ? 1 : 0) << '\n'; }

	template <typename TEnum> static void WriteEnum(std::ofstream& output, std::string_view key, TEnum value)
	{
		output << key << '=' << static_cast<int>(value) << '\n';
	}
};

EditorViewportSettings::EditorViewportSettings() :
    EditorViewportSettings(GetDefaultPath())
{
}

EditorViewportSettings::EditorViewportSettings(std::filesystem::path path) :
    m_path(std::move(path))
{
	(void) Reload();
}

std::filesystem::path EditorViewportSettings::GetDefaultPath()
{
	return Filesystem::GetWorkspaceRootPath() / "Saved" / "Config" / "EditorViewport.ini";
}

void EditorViewportSettings::SanitizeExposure(ViewportExposureOverrides& exposure) noexcept
{
	if (exposure.Mode != EngineExposureMode::Manual && exposure.Mode != EngineExposureMode::Automatic)
	{
		exposure.Mode = EngineExposureMode::Automatic;
	}
	if (exposure.MeteringMethod != EngineExposureMeteringMethod::ParallelReduction
	    && exposure.MeteringMethod != EngineExposureMeteringMethod::DownsamplePyramid)
	{
		exposure.MeteringMethod = EngineExposureMeteringMethod::ParallelReduction;
	}
	exposure.ManualExposure = (std::max) (exposure.ManualExposure, 0.0f);
	exposure.Compensation = std::clamp(exposure.Compensation, -16.0f, 16.0f);
	exposure.TargetLuminance = (std::max) (exposure.TargetLuminance, 0.0001f);
	exposure.Minimum = (std::max) (exposure.Minimum, 0.0f);
	exposure.Maximum = (std::max) (exposure.Maximum, exposure.Minimum);
	exposure.AdaptationSpeedUp = (std::max) (exposure.AdaptationSpeedUp, 0.0f);
	exposure.AdaptationSpeedDown = (std::max) (exposure.AdaptationSpeedDown, 0.0f);
}

void EditorViewportSettings::Sanitize(EditorViewportSettingsState& state) noexcept
{
	state.Navigation.MinimumMoveSpeedMetersPerSecond = (std::max) (state.Navigation.MinimumMoveSpeedMetersPerSecond, 0.0001f);
	state.Navigation.MaximumMoveSpeedMetersPerSecond =
	    (std::max) (state.Navigation.MaximumMoveSpeedMetersPerSecond, state.Navigation.MinimumMoveSpeedMetersPerSecond);
	state.Navigation.MoveSpeedMetersPerSecond = std::clamp(
	    state.Navigation.MoveSpeedMetersPerSecond,
	    state.Navigation.MinimumMoveSpeedMetersPerSecond,
	    state.Navigation.MaximumMoveSpeedMetersPerSecond);
	state.Navigation.RotationSpeedDegreesPerPixel = std::clamp(state.Navigation.RotationSpeedDegreesPerPixel, 0.001f, 10.0f);
	state.Navigation.SprintMultiplier = (std::max) (state.Navigation.SprintMultiplier, 1.0f);
	state.OrthographicHeightMeters = std::clamp(state.OrthographicHeightMeters, 0.001f, 1000000.0f);
	if (state.ProjectionKind != CameraProjectionKind::Perspective && state.ProjectionKind != CameraProjectionKind::Orthographic)
	{
		state.ProjectionKind = CameraProjectionKind::Perspective;
	}
	SanitizeExposure(state.Exposure);
}

bool EditorViewportSettings::SetMoveSpeed(float speedMetersPerSecond) noexcept
{
	speedMetersPerSecond = std::clamp(
	    speedMetersPerSecond,
	    m_state.Navigation.MinimumMoveSpeedMetersPerSecond,
	    m_state.Navigation.MaximumMoveSpeedMetersPerSecond);
	if (speedMetersPerSecond == m_state.Navigation.MoveSpeedMetersPerSecond)
	{
		return true;
	}
	m_state.Navigation.MoveSpeedMetersPerSecond = speedMetersPerSecond;
	return Save();
}

bool EditorViewportSettings::SetRotationSpeed(float degreesPerPixel) noexcept
{
	degreesPerPixel = std::clamp(degreesPerPixel, 0.001f, 10.0f);
	if (degreesPerPixel == m_state.Navigation.RotationSpeedDegreesPerPixel)
	{
		return true;
	}
	m_state.Navigation.RotationSpeedDegreesPerPixel = degreesPerPixel;
	return Save();
}

bool EditorViewportSettings::SetInvertY(bool invertY) noexcept
{
	if (invertY == m_state.Navigation.InvertY)
	{
		return true;
	}
	m_state.Navigation.InvertY = invertY;
	return Save();
}

bool EditorViewportSettings::SetProjectionKind(CameraProjectionKind projectionKind) noexcept
{
	if (projectionKind != CameraProjectionKind::Perspective && projectionKind != CameraProjectionKind::Orthographic)
	{
		projectionKind = CameraProjectionKind::Perspective;
	}
	if (projectionKind == m_state.ProjectionKind)
	{
		return true;
	}
	m_state.ProjectionKind = projectionKind;
	return Save();
}

bool EditorViewportSettings::SetOrthographicHeight(float heightMeters) noexcept
{
	heightMeters = std::clamp(heightMeters, 0.001f, 1000000.0f);
	if (heightMeters == m_state.OrthographicHeightMeters)
	{
		return true;
	}
	m_state.OrthographicHeightMeters = heightMeters;
	return Save();
}

bool EditorViewportSettings::SetExposureOverrides(ViewportExposureOverrides overrides) noexcept
{
	SanitizeExposure(overrides);
	if (overrides == m_state.Exposure)
	{
		return true;
	}
	m_state.Exposure = overrides;
	return Save();
}

bool EditorViewportSettings::Reload() noexcept
{
	EditorViewportSettingsState loaded;
	std::ifstream input(m_path);
	if (!input.is_open())
	{
		Sanitize(loaded);
		m_state = loaded;
		return true;
	}

	bool inSettingsSection = false;
	for (std::string line; std::getline(input, line);)
	{
		const std::string trimmed = Strings::TrimCopy(line);
		if (trimmed.empty() || trimmed.starts_with(';') || trimmed.starts_with('#'))
		{
			continue;
		}
		if (trimmed.starts_with('[') && trimmed.ends_with(']'))
		{
			inSettingsSection = trimmed == "[EditorViewport]";
			continue;
		}
		if (!inSettingsSection)
		{
			continue;
		}
		const std::size_t separator = trimmed.find('=');
		if (separator != std::string::npos)
		{
			EditorViewportSettingsSerialization::Apply(
			    std::string_view(trimmed).substr(0, separator),
			    std::string_view(trimmed).substr(separator + 1),
			    loaded);
		}
	}
	Sanitize(loaded);
	m_state = loaded;
	return true;
}

bool EditorViewportSettings::Save() const noexcept
{
	std::error_code errorCode;
	std::filesystem::create_directories(m_path.parent_path(), errorCode);
	if (errorCode)
	{
		return false;
	}

	std::ofstream output(m_path, std::ios::trunc);
	if (!output.is_open())
	{
		return false;
	}

	output << "[EditorViewport]\n";
	output << "MoveSpeedMetersPerSecond=" << m_state.Navigation.MoveSpeedMetersPerSecond << '\n';
	output << "RotationSpeedDegreesPerPixel=" << m_state.Navigation.RotationSpeedDegreesPerPixel << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "InvertY", m_state.Navigation.InvertY);
	EditorViewportSettingsSerialization::WriteEnum(output, "ProjectionKind", m_state.ProjectionKind);
	output << "OrthographicHeightMeters=" << m_state.OrthographicHeightMeters << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureMode", m_state.Exposure.OverrideMode);
	EditorViewportSettingsSerialization::WriteEnum(output, "ExposureMode", m_state.Exposure.Mode);
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureMeteringMethod", m_state.Exposure.OverrideMeteringMethod);
	EditorViewportSettingsSerialization::WriteEnum(output, "ExposureMeteringMethod", m_state.Exposure.MeteringMethod);
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideManualExposure", m_state.Exposure.OverrideManualExposure);
	output << "ManualExposure=" << m_state.Exposure.ManualExposure << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureCompensation", m_state.Exposure.OverrideCompensation);
	output << "ExposureCompensation=" << m_state.Exposure.Compensation << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureTargetLuminance", m_state.Exposure.OverrideTargetLuminance);
	output << "ExposureTargetLuminance=" << m_state.Exposure.TargetLuminance << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureMinimum", m_state.Exposure.OverrideMinimum);
	output << "ExposureMinimum=" << m_state.Exposure.Minimum << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureMaximum", m_state.Exposure.OverrideMaximum);
	output << "ExposureMaximum=" << m_state.Exposure.Maximum << '\n';
	EditorViewportSettingsSerialization::WriteBool(output, "OverrideExposureAdaptationSpeedUp", m_state.Exposure.OverrideAdaptationSpeedUp);
	output << "ExposureAdaptationSpeedUp=" << m_state.Exposure.AdaptationSpeedUp << '\n';
	EditorViewportSettingsSerialization::WriteBool(
	    output,
	    "OverrideExposureAdaptationSpeedDown",
	    m_state.Exposure.OverrideAdaptationSpeedDown);
	output << "ExposureAdaptationSpeedDown=" << m_state.Exposure.AdaptationSpeedDown << '\n';
	return output.good();
}
