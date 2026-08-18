#include "PCH.h"
#include "Settings/EngineRenderingSettingsPersistence.h"

#include "Core/Public/FileSystemUtils.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Renderer/Public/Settings/EngineRenderingSettings.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

std::filesystem::path EngineRenderingSettingsPersistence::GetConfigPath()
{
	return Filesystem::GetWorkspaceRootPath() / "Config" / "DefaultEngine.ini";
}

std::span<const std::string_view> EngineRenderingSettingsPersistence::GetPersistedNames() noexcept
{
	static constexpr std::string_view persistedNames[] = {
	    "r.VSync",
	    "r.BackBufferFormat",
	    "r.PreferHighPerformanceAdapter",
	    "r.ToneMapper",
	    "r.Exposure.Mode",
	    "r.Exposure.MeteringMethod",
	    "r.OutputColorEncoding",
	    "r.Exposure.Manual",
	    "r.Exposure.Compensation",
	    "r.Exposure.TargetLuminance",
	    "r.Exposure.Min",
	    "r.Exposure.Max",
	    "r.Exposure.AdaptationSpeedUp",
	    "r.Exposure.AdaptationSpeedDown",
	    "r.MeshAutoBatching",
	    "r.Upscaler.Provider",
	    "r.Upscaler.QualityMode",
	    "r.RayReconstruction.Mode",
	    "r.GBuffer.Mode",
	    "r.Lighting.Mode",
	    "r.RayTracing.Tlas.Refit",
	    "r.RayTracing.PreferPartitionedTlas",
	    "r.RayTracing.Ptlas.PartitionsPerAxis",
	    "r.RayTracing.Ptlas.PartitionUpdateMode",
	    "r.RayTracing.Ptlas.MarkAllDynamicInPartition",
	    "r.RayTracing.Ptlas.ModeChangeDistance"};
	return persistedNames;
}

void EngineRenderingSettingsPersistence::Load(LoadVisitor visitor)
{
	if (visitor == nullptr)
	{
		return;
	}
	std::ifstream input(GetConfigPath());
	if (!input.is_open())
	{
		return;
	}

	bool inTargetSection = false;
	for (std::string line; std::getline(input, line);)
	{
		const std::string trimmed = Strings::TrimCopy(line);
		if (trimmed.empty() || trimmed.starts_with(';') || trimmed.starts_with('#'))
		{
			continue;
		}

		if (trimmed.starts_with('[') && trimmed.ends_with(']'))
		{
			inTargetSection = trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == GetSectionName();
			continue;
		}
		if (!inTargetSection)
		{
			continue;
		}

		const std::size_t separator = trimmed.find('=');
		if (separator != std::string::npos)
		{
			visitor(std::string_view(trimmed).substr(0, separator), std::string_view(trimmed).substr(separator + 1));
		}
	}
}

void EngineRenderingSettingsPersistence::Write(const EngineRenderingSettingsState& state)
{
	const std::filesystem::path configPath = GetConfigPath();
	std::error_code errorCode;
	std::filesystem::create_directories(configPath.parent_path(), errorCode);

	std::vector<std::string> lines;
	{
		std::ifstream input(configPath);
		for (std::string line; std::getline(input, line);)
		{
			lines.push_back(std::move(line));
		}
	}

	std::vector<std::string> sectionLines;
	sectionLines.reserve(GetPersistedNames().size() + 1u);
	sectionLines.emplace_back("[" + std::string(GetSectionName()) + "]");
	const auto appendConfigValue = [&sectionLines]<typename TValue>(std::string_view key, TValue value)
	{
		std::string persistedValue;
		if constexpr (std::is_enum_v<TValue>)
		{
			persistedValue = std::to_string(static_cast<std::underlying_type_t<TValue>>(value));
		}
		else if constexpr (std::is_same_v<TValue, bool>)
		{
			persistedValue = value ? "1" : "0";
		}
		else
		{
			persistedValue = std::to_string(value);
		}
		sectionLines.emplace_back(std::string(key) + "=" + persistedValue);
	};
	appendConfigValue("r.VSync", state.VSync);
	appendConfigValue("r.BackBufferFormat", state.BackBufferFormat);
	appendConfigValue("r.PreferHighPerformanceAdapter", state.PreferHighPerformanceAdapter);
	appendConfigValue("r.ToneMapper", state.ToneMapper);
	appendConfigValue("r.Exposure.Mode", state.ExposureMode);
	appendConfigValue("r.Exposure.MeteringMethod", state.ExposureMeteringMethod);
	appendConfigValue("r.OutputColorEncoding", state.OutputColorEncoding);
	appendConfigValue("r.Exposure.Manual", state.ManualExposure);
	appendConfigValue("r.Exposure.Compensation", state.ExposureCompensation);
	appendConfigValue("r.Exposure.TargetLuminance", state.ExposureTargetLuminance);
	appendConfigValue("r.Exposure.Min", state.ExposureMin);
	appendConfigValue("r.Exposure.Max", state.ExposureMax);
	appendConfigValue("r.Exposure.AdaptationSpeedUp", state.ExposureAdaptationSpeedUp);
	appendConfigValue("r.Exposure.AdaptationSpeedDown", state.ExposureAdaptationSpeedDown);
	appendConfigValue("r.MeshAutoBatching", state.MeshAutoBatching);
	appendConfigValue("r.Upscaler.Provider", state.UpscalerProvider);
	appendConfigValue("r.Upscaler.QualityMode", state.UpscalerQualityMode);
	appendConfigValue("r.RayReconstruction.Mode", state.RayReconstructionMode);
	appendConfigValue("r.GBuffer.Mode", state.GBuffer);
	appendConfigValue("r.Lighting.Mode", state.Lighting);
	appendConfigValue("r.RayTracing.Tlas.Refit", state.RefitTlas);
	appendConfigValue("r.RayTracing.PreferPartitionedTlas", state.PtlasActive);
	appendConfigValue("r.RayTracing.Ptlas.PartitionsPerAxis", state.PtlasPartitionsPerAxis);
	appendConfigValue("r.RayTracing.Ptlas.PartitionUpdateMode", state.PtlasPartitionUpdateMode);
	appendConfigValue("r.RayTracing.Ptlas.MarkAllDynamicInPartition", state.PtlasMarkAllDynamicInPartition);
	appendConfigValue("r.RayTracing.Ptlas.ModeChangeDistance", state.PtlasModeChangeDistance);

	std::size_t sectionStart = lines.size();
	std::size_t sectionEnd = lines.size();
	for (std::size_t index = 0; index < lines.size(); ++index)
	{
		const std::string trimmed = Strings::TrimCopy(lines[index]);
		if (!trimmed.starts_with('[') || !trimmed.ends_with(']'))
		{
			continue;
		}
		if (trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == GetSectionName())
		{
			sectionStart = index;
			sectionEnd = index + 1;
			while (sectionEnd < lines.size())
			{
				const std::string nextTrimmed = Strings::TrimCopy(lines[sectionEnd]);
				if (nextTrimmed.starts_with('[') && nextTrimmed.ends_with(']'))
				{
					break;
				}
				++sectionEnd;
			}
			break;
		}
	}

	if (sectionStart < lines.size())
	{
		lines.erase(lines.begin() + static_cast<std::ptrdiff_t>(sectionStart), lines.begin() + static_cast<std::ptrdiff_t>(sectionEnd));
		lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(sectionStart), sectionLines.begin(), sectionLines.end());
	}
	else
	{
		if (!lines.empty() && !Strings::TrimCopy(lines.back()).empty())
		{
			lines.emplace_back();
		}
		lines.insert(lines.end(), sectionLines.begin(), sectionLines.end());
	}

	std::ofstream output(configPath, std::ios::trunc);
	for (const std::string& line : lines)
	{
		output << line << '\n';
	}
}

bool EngineRenderingSettingsPersistence::IsPersistedName(std::string_view name) noexcept
{
	for (const std::string_view persistedName : GetPersistedNames())
	{
		if (name == persistedName)
		{
			return true;
		}
	}
	return false;
}
