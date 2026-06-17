#include "PCH.h"

#include "Settings/EditorSettingsService.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

std::filesystem::path EditorSettingsDetail::GetEditorSettingsConfigPath()
{
	return Paths::WorkspaceRoot() / "Config" / "DefaultEditor.ini";
}

void EditorSettingsDetail::LoadConfigSectionValues(
    const std::filesystem::path& configPath,
    std::string_view sectionName,
    const std::function<void(std::string_view key, std::string_view value)>& onValue)
{
	std::ifstream input(configPath);
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
			inTargetSection = trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == sectionName;
			continue;
		}

		if (!inTargetSection)
		{
			continue;
		}

		const std::size_t separator = trimmed.find('=');
		if (separator == std::string::npos)
		{
			continue;
		}

		onValue(
		    std::string_view(trimmed).substr(0, separator),
		    std::string_view(trimmed).substr(separator + 1));
	}
}

void EditorSettingsDetail::WriteConfigSectionValues(
    const std::filesystem::path& configPath,
    std::string_view sectionName,
    const std::vector<std::pair<std::string, std::string>>& values)
{
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
	sectionLines.emplace_back("[" + std::string(sectionName) + "]");
	for (const auto& [key, value] : values)
	{
		sectionLines.emplace_back(key + "=" + value);
	}

	std::size_t sectionStart = lines.size();
	std::size_t sectionEnd = lines.size();
	for (std::size_t index = 0; index < lines.size(); ++index)
	{
		const std::string trimmed = Strings::TrimCopy(lines[index]);
		if (!trimmed.starts_with('[') || !trimmed.ends_with(']'))
		{
			continue;
		}

		if (trimmed.size() > 2 && trimmed.substr(1, trimmed.size() - 2) == sectionName)
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
