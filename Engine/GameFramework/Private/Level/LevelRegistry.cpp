#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"
#include "Level/LevelRegistry.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "Level/Level.h"
#include "Level/Parsing/LevelParser.h"

#include <format>

static const auto g_levelRegistryLogger = Logging::GetOrCreateLogger("GameFramework.LevelRegistry");

LevelRegistry::LevelRegistry()
{
	DiscoverLevels();
}

LevelRegistry::~LevelRegistry() noexcept = default;

void LevelRegistry::DiscoverLevels()
{
	const std::filesystem::path projectRoot = Filesystem::GetProjectPath();
	try
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(projectRoot);
		std::string startupDefaultLevelName;
		for (const ProjectLevelCatalogEntry& entry : catalog.levels)
		{
			if (!catalog.IsLevelReady(projectRoot, entry))
			{
				if (entry.defaultIncluded || entry.startupDefault)
				{
					Diagnostics::Fatal(
					    g_levelRegistryLogger,
					    __FILE__,
					    __LINE__,
					    std::format(
					        "LevelRegistry: active level '{}' is not ready: '{}'.",
					        entry.id,
					        entry.sourcePath.string()));
				}
				continue;
			}

			LoadCatalogLevel(entry, startupDefaultLevelName);
		}

		ResolveDefaultLevel(startupDefaultLevelName);
	}
	catch (const Diagnostics::Error& error)
	{
		Diagnostics::Fatal(g_levelRegistryLogger, __FILE__, __LINE__, std::string("LevelRegistry: ") + error.what());
	}
}

void LevelRegistry::LoadCatalogLevel(const ProjectLevelCatalogEntry& entry, std::string& outStartupDefaultLevelName)
{
	try
	{
		auto loadedLevel = LevelParser::LoadFromFile(entry.sourcePath);
		const std::string loadedLevelName(loadedLevel->GetName());
		Register(std::move(loadedLevel));

		if (entry.startupDefault)
			outStartupDefaultLevelName = loadedLevelName;
	}
	catch (const Diagnostics::Error& error)
	{
		const std::string failure =
		    std::format("LevelRegistry: Failed to load catalog level '{}': {}", entry.sourcePath.string(), error.what());
		if (entry.defaultIncluded || entry.startupDefault)
			Diagnostics::Fatal(g_levelRegistryLogger, __FILE__, __LINE__, failure);
		SPDLOG_LOGGER_WARN(g_levelRegistryLogger, "{}", failure);
	}
}

void LevelRegistry::ResolveDefaultLevel(std::string_view startupDefaultLevelName)
{
	if (!startupDefaultLevelName.empty())
	{
		SetDefaultLevelName(startupDefaultLevelName);
	}
	else
	{
		Diagnostics::Fatal(g_levelRegistryLogger, __FILE__, __LINE__, "LevelRegistry: No ready level is explicitly marked StartupDefault.");
	}
}

void LevelRegistry::Register(std::unique_ptr<LevelAsset> level)
{
	if (!level)
		Diagnostics::Fatal(g_levelRegistryLogger, __FILE__, __LINE__, "LevelRegistry: Attempted to register a null level.");

	auto name = level->GetName();
	std::string nameKey(name);

	if (m_levels.contains(nameKey))
		Diagnostics::Fatal(
		    g_levelRegistryLogger,
		    __FILE__,
		    __LINE__,
		    std::format("LevelRegistry: Duplicate level name '{}'.", nameKey));

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

void LevelRegistry::SaveLevel(const LevelAsset& level) const
{
	if (FindLevel(level.GetName()) == nullptr)
		throw Diagnostics::Error("Level not found.");

	LevelParser::SaveToFile(level);
}

std::string_view LevelRegistry::GetDefaultLevelName() const noexcept
{
	return m_defaultLevelName;
}
