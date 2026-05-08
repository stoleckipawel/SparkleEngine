#pragma once

#include <filesystem>

namespace AssetAuthoring
{
	class TextureCookerCommand
	{
	  public:
		virtual ~TextureCookerCommand() = default;

		virtual int Execute(const std::filesystem::path& requestFilePath) const = 0;
	};
}