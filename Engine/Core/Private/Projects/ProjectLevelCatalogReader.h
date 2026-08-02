#pragma once

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>
#include <unordered_set>

class ProjectLevelCatalogReader final
{
public:
	static ProjectLevelCatalog Read(const std::filesystem::path& projectRoot);
	static void ValidateText(const std::filesystem::path& projectRoot, std::string_view text);

private:
	enum class Section
	{
		None,
		Level,
		AssetPack
	};

	explicit ProjectLevelCatalogReader(const std::filesystem::path& projectRoot) noexcept;

	void ReadCatalog(std::istream& input);
	void ParseLine(std::string line);
	void BeginLevel();
	void BeginAssetPack();
	void ParseLevelField(std::string_view key, std::string_view value);
	void ParseAssetPackField(std::string_view key, std::string_view value);
	bool ParseBool(std::string_view value) const;
	std::uintmax_t ParseByteCount(std::string_view value) const;
	void ValidateCurrentSection() const;
	void ValidateCatalog() const;
	std::filesystem::path ResolveProjectPath(std::string_view value) const;
	static bool IsSafeIdentifier(std::string_view value) noexcept;
	static bool IsSha256(std::string_view value) noexcept;

	const std::filesystem::path& m_projectRoot;
	ProjectLevelCatalog m_catalog;
	Section m_section = Section::None;
	ProjectLevelCatalogEntry* m_currentLevel = nullptr;
	ProjectAssetPack* m_currentPack = nullptr;
	std::unordered_set<std::string> m_sectionFields;
	std::size_t m_lineNumber = 0;
};
