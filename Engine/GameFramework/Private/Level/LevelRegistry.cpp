#include "PCH.h"
#include "Level/LevelRegistry.h"
#include "Level/Level.h"

#include "Level/Levels/EmptyLevel.h"
#include "Level/Levels/BasicShapesLevel.h"
#include "Level/Levels/ABeautifulGameLevel.h"
#include "Level/Levels/CesiumManLevel.h"
#include "Level/Levels/DamagedHelmetLevel.h"
#include "Level/Levels/DiffuseTransmissionPlantLevel.h"
#include "Level/Levels/SponzaLevel.h"

#include "Core/Public/Diagnostics/Log.h"

// =============================================================================
// Lifecycle
// =============================================================================

LevelRegistry::LevelRegistry()
{
	RegisterBuiltinLevels();
}

LevelRegistry::~LevelRegistry() noexcept = default;

// =============================================================================
// Built-in Levels
// =============================================================================

void LevelRegistry::RegisterBuiltinLevels()
{
	Register(std::make_unique<EmptyLevel>());
	Register(std::make_unique<BasicShapesLevel>());
	Register(std::make_unique<ABeautifulGameLevel>());
	Register(std::make_unique<CesiumManLevel>());
	Register(std::make_unique<DamagedHelmetLevel>());
	Register(std::make_unique<DiffuseTransmissionPlantLevel>());
	Register(std::make_unique<SponzaLevel>());

	SetDefaultLevelName("BasicShapes");
}

// =============================================================================
// Registration
// =============================================================================

void LevelRegistry::Register(std::unique_ptr<Level> level)
{
	if (!level)
	{
		LOG_WARNING("LevelRegistry: Attempted to register a null level");
		return;
	}

	auto name = level->GetName();
	std::string nameKey(name);

	if (m_levels.contains(nameKey))
	{
		LOG_WARNING("LevelRegistry: Duplicate level name '" + nameKey + "' — skipping");
		return;
	}

	LOG_INFO("LevelRegistry: Registered level '" + nameKey + "'");
	m_levels.emplace(std::move(nameKey), std::move(level));
}

// =============================================================================
// Lookup
// =============================================================================

Level* LevelRegistry::FindLevel(std::string_view name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto it = m_levels.find(std::string(name));
	return it != m_levels.end() ? it->second.get() : nullptr;
}

Level* LevelRegistry::FindLevelOrDefault(std::string_view name) const
{
	if (!name.empty())
	{
		if (auto* level = FindLevel(name))
		{
			return level;
		}
		LOG_WARNING("LevelRegistry: Level '" + std::string(name) + "' not found — falling back to default");
	}

	if (auto* level = GetDefaultLevel())
	{
		return level;
	}

	LOG_WARNING("LevelRegistry: No default level available");
	return nullptr;
}

const std::unordered_map<std::string, std::unique_ptr<Level>>& LevelRegistry::GetAllLevels() const noexcept
{
	return m_levels;
}

std::size_t LevelRegistry::GetLevelCount() const noexcept
{
	return m_levels.size();
}

// =============================================================================
// Default Level
// =============================================================================

void LevelRegistry::SetDefaultLevelName(std::string_view name)
{
	m_defaultLevelName = std::string(name);
}

std::string_view LevelRegistry::GetDefaultLevelName() const noexcept
{
	return m_defaultLevelName;
}

Level* LevelRegistry::GetDefaultLevel() const
{
	return FindLevel(m_defaultLevelName);
}
