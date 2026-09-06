#pragma once

#include <filesystem>

class TextureCookerCommand
{
public:
	virtual ~TextureCookerCommand() = default;
	TextureCookerCommand(const TextureCookerCommand&) = delete;
	TextureCookerCommand& operator=(const TextureCookerCommand&) = delete;
	TextureCookerCommand(TextureCookerCommand&&) = delete;
	TextureCookerCommand& operator=(TextureCookerCommand&&) = delete;

	virtual int Execute(const std::filesystem::path& requestFilePath) const = 0;

protected:
	TextureCookerCommand() = default;
};
