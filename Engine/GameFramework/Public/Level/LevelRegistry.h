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
	LevelRegistry();
	~LevelRegistry() noexcept;

	LevelRegistry(const LevelRegistry&) = delete;
	LevelRegistry& operator=(const LevelRegistry&) = delete;
	LevelRegistry(LevelRegistry&&) = delete;
	LevelRegistry& operator=(LevelRegistry&&) = delete;

	void Register(std::unique_ptr<Level> level);

	Level* FindLevel(std::string_view name) const;

	Level* FindLevelOrDefault(std::string_view name) const;

	const std::unordered_map<std::string, std::unique_ptr<Level>>& GetAllLevels() const noexcept;

	std::size_t GetLevelCount() const noexcept;

	void SetDefaultLevelName(std::string_view name);

	std::string_view GetDefaultLevelName() const noexcept;
	Level* GetDefaultLevel() const;

  private:
	void RegisterBuiltinLevels();

	std::unordered_map<std::string, std::unique_ptr<Level>> m_levels;
	std::string m_defaultLevelName;
};
