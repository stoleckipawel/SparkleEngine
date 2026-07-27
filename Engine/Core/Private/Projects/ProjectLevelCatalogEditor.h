#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

class ProjectLevelCatalogEditor final
{
  public:
	static bool SetEntryBool(
	    const std::filesystem::path& projectRoot,
	    std::string_view sectionHeader,
	    std::string_view entryId,
	    std::string_view keyName,
	    bool value,
	    std::string& outErrorMessage);

  private:
	ProjectLevelCatalogEditor(
	    std::filesystem::path catalogPath,
	    std::string sectionHeader,
	    std::string entryId,
	    std::string keyName,
	    bool value);

	bool Apply(std::string& outErrorMessage);
	bool Read(std::string& outErrorMessage);
	bool UpdateEntry(std::string& outErrorMessage);
	bool Publish(std::string& outErrorMessage) const;
	std::string BuildText() const;

	std::filesystem::path m_catalogPath;
	std::string m_sectionHeader;
	std::string m_entryId;
	std::string m_keyName;
	std::string m_valueLine;
	std::vector<std::string> m_lines;
};
