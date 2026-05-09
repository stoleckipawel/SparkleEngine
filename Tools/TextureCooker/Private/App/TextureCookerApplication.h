#pragma once

#include <filesystem>

	class TextureCookerApplication final
	{
	  public:
		int Run(int argc, char** argv) const;

	  private:
		static int RunCommand(const std::filesystem::path& requestFilePath, const char* commandName);
	};