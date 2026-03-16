#pragma once

#include <filesystem>
#include <memory>
#include <string>

class Level;

class LevelParser final
{
  public:
	static std::unique_ptr<Level> LoadFromFile(const std::filesystem::path& filePath, std::string& errorMessage);
	static bool SaveToFile(const Level& level, std::string* errorMessage = nullptr);

	LevelParser() = delete;
	~LevelParser() = delete;
	LevelParser(const LevelParser&) = delete;
	LevelParser& operator=(const LevelParser&) = delete;
	LevelParser(LevelParser&&) = delete;
	LevelParser& operator=(LevelParser&&) = delete;
};