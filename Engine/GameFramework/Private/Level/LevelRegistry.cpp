#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"
#include "Level/LevelRegistry.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "Level/Level.h"
#include "Level/Parsing/LevelParser.h"

static const auto g_levelRegistryLogger = Logging::GetOrCreateLogger("GameFramework.LevelRegistry");

LevelRegistry::LevelRegistry()
{
	DiscoverLevels();
}

LevelRegistry::~LevelRegistry() noexcept = default;

void LevelRegistry::DiscoverLevels()
{
	const std::filesystem::path projectRoot = Filesystem::GetProjectPath();
	ProjectLevelCatalog catalog;
	std::string catalogError;
	if (ProjectLevelCatalogFile::Load(projectRoot, catalog, catalogError))
	{
		std::string startupDefaultLevelName;
		for (const ProjectLevelCatalogEntry& entry : catalog.levels)
		{
			if (!catalog.IsLevelReady(projectRoot, entry))
			{
				continue;
			}

			LoadCatalogLevel(entry, startupDefaultLevelName);
		}

		ResolveDefaultLevel(startupDefaultLevelName);
		return;
	}

	SPDLOG_LOGGER_WARN(
	    g_levelRegistryLogger,
	    "LevelRegistry: {}",
	    catalogError);
}

void LevelRegistry::LoadCatalogLevel(
    const ProjectLevelCatalogEntry& entry,
    std::string& outStartupDefaultLevelName)
{
	std::string errorMessage;
	auto loadedLevel = LevelParser::LoadFromFile(entry.sourcePath, errorMessage);
	if (!loadedLevel)
	{
		SPDLOG_LOGGER_WARN(
		    g_levelRegistryLogger,
		    "LevelRegistry: Failed to load catalog level '{}'{}",
		    entry.sourcePath.string(),
		    errorMessage.empty() ? std::string() : std::string{" - "} + errorMessage);
		return;
	}

	if (entry.startupDefault)
	{
		outStartupDefaultLevelName = std::string(loadedLevel->GetName());
	}

	Register(std::move(loadedLevel));
}

void LevelRegistry::ResolveDefaultLevel(std::string_view startupDefaultLevelName)
{
	if (!startupDefaultLevelName.empty())
	{
		SetDefaultLevelName(startupDefaultLevelName);
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

std::vector<std::string> LevelRegistry::GetLevelNames() const
{
	std::vector<std::string> levelNames;
	levelNames.reserve(m_levels.size());
	for (const auto& levelEntry : m_levels)
	{
		levelNames.push_back(levelEntry.first);
	}
	std::sort(levelNames.begin(), levelNames.end());
	return levelNames;
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
