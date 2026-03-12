// ============================================================================
// LevelRegistry.h
// ----------------------------------------------------------------------------
// Central registry for all available levels. Engine registers built-in levels
// automatically; applications register custom levels at startup.
//
// USAGE:
//   LevelRegistry registry;
//   registry.Register(std::make_unique<MyCustomLevel>());
//   Level* level = registry.FindLevel("Sponza");
//
// DESIGN:
//   - Owned object — created and managed by App (no singleton)
//   - Built-in engine levels registered in constructor (Empty, BasicShapes, Sponza)
//   - Applications register additional levels before App::Run()
//   - Duplicate names are rejected with a warning
//   - Default level name determines what Scene::LoadLevel loads on startup
//
// ============================================================================

#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class Level;

class SPARKLE_ENGINE_API LevelRegistry final
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	LevelRegistry();
	~LevelRegistry() noexcept;

	LevelRegistry(const LevelRegistry&) = delete;
	LevelRegistry& operator=(const LevelRegistry&) = delete;
	LevelRegistry(LevelRegistry&&) = delete;
	LevelRegistry& operator=(LevelRegistry&&) = delete;

	// ========================================================================
	// Registration
	// ========================================================================

	/// Registers a level. Rejects duplicates (logs a warning).
	void Register(std::unique_ptr<Level> level);

	// ========================================================================
	// Lookup
	// ========================================================================

	/// Finds a registered level by name. Returns nullptr if not found.
	Level* FindLevel(std::string_view name) const;

	/// Finds by name if non-empty, otherwise returns the default level.
	/// Logs a warning and returns nullptr if neither resolves.
	Level* FindLevelOrDefault(std::string_view name) const;

	const std::unordered_map<std::string, std::unique_ptr<Level>>& GetAllLevels() const noexcept;
	
	std::size_t GetLevelCount() const noexcept;

	// ========================================================================
	// Default Level
	// ========================================================================


	void SetDefaultLevelName(std::string_view name);
	
	std::string_view GetDefaultLevelName() const noexcept;
	Level* GetDefaultLevel() const;

  private:
	void RegisterBuiltinLevels();

	std::unordered_map<std::string, std::unique_ptr<Level>> m_levels;
	std::string m_defaultLevelName;
};
