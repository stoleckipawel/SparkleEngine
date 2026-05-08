#pragma once

#include <filesystem>

namespace AssetAuthoring
{
	class TextureCookerApplication final
	{
	  public:
		int Run(int argc, char** argv) const;

	  private:
		static int RunCommand(const std::filesystem::path& requestFilePath, const char* commandName);
	};
}