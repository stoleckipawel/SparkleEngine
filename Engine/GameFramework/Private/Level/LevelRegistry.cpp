#include "PCH.h"
#include "Level/LevelRegistry.h"

#include "Core/Public/FileSystemUtils.h"
#include "Level/Level.h"
#include "Level/Parsing/LevelParser.h"

#include <algorithm>

static const auto g_levelRegistryLogger = Engine::Logging::GetOrCreateLogger("GameFramework.LevelRegistry");

LevelRegistry::LevelRegistry()
{
	DiscoverLevels();
}

LevelRegistry::~LevelRegistry() noexcept = default;

void LevelRegistry::DiscoverLevels()
{
	const std::filesystem::path levelsPath = Filesystem::GetProjectPath() / "Levels";
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

	SPDLOG_LOGGER_INFO(g_levelRegistryLogger, "LevelRegistry: Registered level '{}'", nameKey);
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
