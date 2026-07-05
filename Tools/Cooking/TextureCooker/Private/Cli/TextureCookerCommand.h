#pragma once

#include <filesystem>

	class TextureCookerCommand
	{
	  public:
		virtual ~TextureCookerCommand() = default;

		virtual int Execute(const std::filesystem::path& requestFilePath) const = 0;
	};
