#pragma once

#include <filesystem>

	struct TextureCookerCommandOptions final
	{
		std::filesystem::path summaryPath;
	};

	class TextureCookerCommand
	{
	  public:
		virtual ~TextureCookerCommand() = default;

		virtual int Execute(const std::filesystem::path& requestFilePath, const TextureCookerCommandOptions& options) const = 0;
	};