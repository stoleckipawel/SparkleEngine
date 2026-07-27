#pragma once

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>

class ProjectLevelCatalogReader final
{
  public:
	static bool Read(
	    const std::filesystem::path& projectRoot,
	    ProjectLevelCatalog& outCatalog,
	    std::string& outErrorMessage);

  private:
	enum class Section
	{
		None,
		Level,
		OptionalContentPack
	};

	ProjectLevelCatalogReader(
	    const std::filesystem::path& projectRoot,
	    ProjectLevelCatalog& catalog) noexcept;

	void ReadCatalog(std::istream& input);
	void ParseLine(std::string line);
	void BeginLevel();
	void BeginOptionalContentPack() noexcept;
	void ParseLevelField(std::string_view key, std::string_view value);
	void ParseOptionalContentPackField(std::string_view key, std::string_view value);
	std::filesystem::path ResolveProjectPath(std::string_view value) const;

	const std::filesystem::path& m_projectRoot;
	ProjectLevelCatalog& m_catalog;
	Section m_section = Section::None;
	ProjectLevelCatalogEntry* m_currentLevel = nullptr;
	ProjectOptionalContentPack* m_currentPack = nullptr;
};
