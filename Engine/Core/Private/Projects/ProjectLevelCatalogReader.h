#pragma once

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_set>

class ProjectLevelCatalogReader final
{
  public:
	static ProjectLevelCatalog Read(const std::filesystem::path& projectRoot);

  private:
	enum class Section
	{
		None,
		Level,
		OptionalContentPack
	};

	explicit ProjectLevelCatalogReader(const std::filesystem::path& projectRoot) noexcept;

	void ReadCatalog(std::istream& input);
	void ParseLine(std::string line);
	void BeginLevel();
	void BeginOptionalContentPack() noexcept;
	void ParseLevelField(std::string_view key, std::string_view value);
	void ParseOptionalContentPackField(std::string_view key, std::string_view value);
	bool ParseBool(std::string_view value) const;
	void ValidateCatalog() const;
	std::filesystem::path ResolveProjectPath(std::string_view value) const;

	const std::filesystem::path& m_projectRoot;
	ProjectLevelCatalog m_catalog;
	Section m_section = Section::None;
	ProjectLevelCatalogEntry* m_currentLevel = nullptr;
	ProjectOptionalContentPack* m_currentPack = nullptr;
	std::unordered_set<std::string> m_sectionFields;
};
