#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class ProjectLevelCatalogEditor final
{
public:
	static bool SetLevelSelected(
	    const std::filesystem::path& projectRoot,
	    std::string_view levelId,
	    bool selected,
	    std::string& outErrorMessage);
	static bool SetLevelsSelected(
	    const std::filesystem::path& projectRoot,
	    const std::vector<std::string>& levelIds,
	    bool selected,
	    std::string& outErrorMessage);

private:
	ProjectLevelCatalogEditor(std::filesystem::path catalogPath, std::vector<std::string> levelIds, bool selected);

	bool Apply(std::string& outErrorMessage);
	bool Read(std::string& outErrorMessage);
	bool UpdateEntries(std::string& outErrorMessage);
	bool Validate(std::string_view text, std::string& outErrorMessage) const;
	bool Publish(std::string_view text, std::string& outErrorMessage) const;
	std::string BuildText() const;

	std::filesystem::path m_catalogPath;
	std::vector<std::string> m_levelIds;
	std::string m_valueLine;
	std::vector<std::string> m_lines;
};
