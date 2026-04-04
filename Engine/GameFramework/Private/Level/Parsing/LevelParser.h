#pragma once

#include <filesystem>
#include <memory>
#include <string>

class LevelAsset;

class LevelParser final
{
  public:
	static std::unique_ptr<LevelAsset> LoadFromFile(const std::filesystem::path& filePath, std::string& errorMessage);
	static bool SaveToFile(const LevelAsset& level, std::string* errorMessage = nullptr);

	LevelParser() = delete;
	~LevelParser() = delete;
	LevelParser(const LevelParser&) = delete;
	LevelParser& operator=(const LevelParser&) = delete;
	LevelParser(LevelParser&&) = delete;
	LevelParser& operator=(LevelParser&&) = delete;
};