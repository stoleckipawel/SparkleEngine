#include "PCH.h"
#include "Level/LevelRegistry.h"

#include "Core/Public/Paths/DirectoryPaths.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Level/Level.h"
#include "Level/Parsing/LevelParser.h"

#include <algorithm>
#include <fstream>
#include <unordered_map>

static const auto g_levelRegistryLogger = Logging::GetOrCreateLogger("GameFramework.LevelRegistry");

namespace
{
	constexpr const char* kLevelCatalogFileName = "Levels.catalog";

	struct LevelCatalogEntry final
	{
		std::filesystem::path sourcePath;
		std::string optionalPackId;
	};

	struct OptionalContentPack final
	{
		std::filesystem::path path;
		bool available = true;
	};

	struct LevelCatalog final
	{
		std::vector<LevelCatalogEntry> levels;
		std::unordered_map<std::string, OptionalContentPack> optionalPacks;
	};

	enum class LevelCatalogSection
	{
		None,
		Level,
		OptionalPack
	};

	std::filesystem::path ResolveProjectPath(const std::filesystem::path& projectRoot, std::string_view value)
	{
		std::filesystem::path path{Strings::UnquoteCopy(value)};
		if (path.is_relative())
		{
			path = projectRoot / path;
		}
		return path.lexically_normal();
	}

	bool OptionalPackAvailable(const LevelCatalog& catalog, const std::filesystem::path& projectRoot, std::string_view optionalPackId)
	{
		if (optionalPackId.empty())
		{
			return true;
		}

		const auto packIt = catalog.optionalPacks.find(std::string(optionalPackId));
		if (packIt == catalog.optionalPacks.end() || !packIt->second.available)
		{
			return false;
		}

		if (packIt->second.path.empty())
		{
			return true;
		}

		const std::filesystem::path packPath =
		    packIt->second.path.is_relative() ? (projectRoot / packIt->second.path) : packIt->second.path;
		std::error_code errorCode;
		return std::filesystem::exists(packPath.lexically_normal(), errorCode);
	}

	bool LoadLevelCatalog(const std::filesystem::path& projectRoot, LevelCatalog& outCatalog)
	{
		const std::filesystem::path catalogPath = projectRoot / kLevelCatalogFileName;
		std::ifstream input(catalogPath);
		if (!input.is_open())
		{
			return false;
		}

		LevelCatalogSection section = LevelCatalogSection::None;
		LevelCatalogEntry* currentLevel = nullptr;
		std::string currentOptionalPackId;
		for (std::string line; std::getline(input, line);)
		{
			line = Strings::TrimCopy(line);
			if (line.empty() || line[0] == '#' || line[0] == ';')
			{
				continue;
			}

			if (line == "[Level]")
			{
				currentOptionalPackId.clear();
				section = LevelCatalogSection::Level;
				currentLevel = &outCatalog.levels.emplace_back();
				continue;
			}
			if (line == "[OptionalPack]")
			{
				currentLevel = nullptr;
				currentOptionalPackId.clear();
				section = LevelCatalogSection::OptionalPack;
				continue;
			}

			std::string_view key;
			std::string_view value;
			if (!Strings::TrySplitKeyValue(line, '=', key, value))
			{
				continue;
			}

			if (section == LevelCatalogSection::Level && currentLevel != nullptr)
			{
				if (key == "Source")
				{
					currentLevel->sourcePath = ResolveProjectPath(projectRoot, value);
				}
				else if (key == "OptionalPack")
				{
					currentLevel->optionalPackId = Strings::UnquoteCopy(value);
				}
				continue;
			}

			if (section == LevelCatalogSection::OptionalPack)
			{
				if (key == "Id")
				{
					currentOptionalPackId = Strings::UnquoteCopy(value);
					outCatalog.optionalPacks.try_emplace(currentOptionalPackId);
				}
				else if (!currentOptionalPackId.empty())
				{
					OptionalContentPack& pack = outCatalog.optionalPacks[currentOptionalPackId];
					if (key == "Root" || key == "Path")
					{
						pack.path = std::filesystem::path{Strings::UnquoteCopy(value)};
					}
					else if (key == "Available")
					{
						bool available = true;
						if (Strings::TryParseBool(value, available))
						{
							pack.available = available;
						}
					}
				}
			}
		}

		return true;
	}
}

LevelRegistry::LevelRegistry()
{
	DiscoverLevels();
}

LevelRegistry::~LevelRegistry() noexcept = default;

void LevelRegistry::DiscoverLevels()
{
	const std::filesystem::path projectRoot = Paths::ProjectRoot();
	LevelCatalog catalog;
	if (LoadLevelCatalog(projectRoot, catalog))
	{
		for (const LevelCatalogEntry& entry : catalog.levels)
		{
			if (entry.sourcePath.empty() || !OptionalPackAvailable(catalog, projectRoot, entry.optionalPackId))
			{
				continue;
			}

			std::string errorMessage;
			auto loadedLevel = LevelParser::LoadFromFile(entry.sourcePath, errorMessage);
			if (!loadedLevel)
			{
				SPDLOG_LOGGER_WARN(
				    g_levelRegistryLogger,
				    "LevelRegistry: Failed to load catalog level '{}'{}",
				    entry.sourcePath.string(),
				    errorMessage.empty() ? std::string() : std::string{" - "} + errorMessage);
				continue;
			}

			Register(std::move(loadedLevel));
		}

		if (FindLevel("Empty") != nullptr)
		{
			SetDefaultLevelName("Empty");
		}
		else if (!m_levels.empty())
		{
			SetDefaultLevelName(m_levels.begin()->first);
		}
		return;
	}

	const std::filesystem::path levelsPath = Paths::ProjectLevelsRoot();
	std::error_code errorCode;
	if (!std::filesystem::exists(levelsPath, errorCode))
	{
		SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: Levels directory not found at '{}'", levelsPath.string());
		return;
	}

	std::vector<std::filesystem::path> levelFiles;
	for (const auto& entry : std::filesystem::directory_iterator(levelsPath, errorCode))
	{
		if (errorCode)
		{
			SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: Failed while scanning levels directory '{}'", levelsPath.string());
			break;
		}

		if (!entry.is_regular_file())
		{
			continue;
		}

		if (entry.path().extension() == ".level")
		{
			levelFiles.push_back(entry.path());
		}
	}

	std::sort(levelFiles.begin(), levelFiles.end());
	for (const std::filesystem::path& levelFile : levelFiles)
	{
		std::string errorMessage;
		auto loadedLevel = LevelParser::LoadFromFile(levelFile, errorMessage);
		if (!loadedLevel)
		{
			SPDLOG_LOGGER_WARN(
			    g_levelRegistryLogger,
			    "LevelRegistry: Failed to load level file '{}'{}",
			    levelFile.string(),
			    errorMessage.empty() ? std::string() : std::string{" - "} + errorMessage);
			continue;
		}

		Register(std::move(loadedLevel));
	}

	if (FindLevel("Empty") != nullptr)
	{
		SetDefaultLevelName("Empty");
	}
	else if (!m_levels.empty())
	{
		SetDefaultLevelName(m_levels.begin()->first);
	}
}

void LevelRegistry::Register(std::unique_ptr<LevelAsset> level)
{
	if (!level)
	{
		SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: Attempted to register a null level");
		return;
	}

	auto name = level->GetName();
	std::string nameKey(name);

	if (m_levels.contains(nameKey))
	{
		SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: Duplicate level name '{}' - skipping", nameKey);
		return;
	}

	m_levels.emplace(std::move(nameKey), std::move(level));
}

LevelAsset* LevelRegistry::FindLevel(std::string_view name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto it = m_levels.find(std::string(name));
	return it != m_levels.end() ? it->second.get() : nullptr;
}

LevelAsset* LevelRegistry::FindLevelOrDefault(std::string_view name) const
{
	if (!name.empty())
	{
		if (auto* level = FindLevel(name))
		{
			return level;
		}
		SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: Level '{}' not found - falling back to default", std::string(name));
	}

	if (auto* level = GetDefaultLevel())
	{
		return level;
	}

	SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "LevelRegistry: No default level available");
	return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<LevelAsset>>& LevelRegistry::GetAllLevels() const noexcept
{
	return m_levels;
}

std::size_t LevelRegistry::GetLevelCount() const noexcept
{
	return m_levels.size();
}

void LevelRegistry::SetDefaultLevelName(std::string_view name)
{
	m_defaultLevelName = std::string(name);
}

bool LevelRegistry::SaveLevel(const LevelAsset& level, std::string* errorMessage) const
{
	if (FindLevel(level.GetName()) == nullptr)
	{
		if (errorMessage != nullptr)
		{
			*errorMessage = "Level not found";
		}
		return false;
	}

	return LevelParser::SaveToFile(level, errorMessage);
}

std::string_view LevelRegistry::GetDefaultLevelName() const noexcept
{
	return m_defaultLevelName;
}

LevelAsset* LevelRegistry::GetDefaultLevel() const
{
	return FindLevel(m_defaultLevelName);
}
