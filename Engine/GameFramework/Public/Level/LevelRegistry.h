#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class LevelAsset;

class SPARKLE_ENGINE_API LevelRegistry final
{
  public:
	LevelRegistry();
	~LevelRegistry() noexcept;

	LevelRegistry(const LevelRegistry&) = delete;
	LevelRegistry& operator=(const LevelRegistry&) = delete;
	LevelRegistry(LevelRegistry&&) = delete;
	LevelRegistry& operator=(LevelRegistry&&) = delete;

	LevelAsset* FindLevel(std::string_view name) const;

	LevelAsset* FindLevelOrDefault(std::string_view name) const;

	const std::unordered_map<std::string, std::unique_ptr<LevelAsset>>& GetAllLevels() const noexcept;

	std::size_t GetLevelCount() const noexcept;

	void SetDefaultLevelName(std::string_view name);
	bool SaveLevel(const LevelAsset& level, std::string* errorMessage = nullptr) const;

	std::string_view GetDefaultLevelName() const noexcept;
	LevelAsset* GetDefaultLevel() const;

  private:
	void DiscoverLevels();
	void Register(std::unique_ptr<LevelAsset> level);

	std::unordered_map<std::string, std::unique_ptr<LevelAsset>> m_levels;
	std::string m_defaultLevelName;
};
