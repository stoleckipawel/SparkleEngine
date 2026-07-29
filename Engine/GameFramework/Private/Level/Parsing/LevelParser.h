#pragma once

#include <filesystem>
#include <memory>

class LevelAsset;

class LevelParser final
{
  public:
	static std::unique_ptr<LevelAsset> LoadFromFile(const std::filesystem::path& filePath);
	static void SaveToFile(const LevelAsset& level);

	LevelParser() = delete;
	~LevelParser() = delete;
	LevelParser(const LevelParser&) = delete;
	LevelParser& operator=(const LevelParser&) = delete;
	LevelParser(LevelParser&&) = delete;
	LevelParser& operator=(LevelParser&&) = delete;
};
