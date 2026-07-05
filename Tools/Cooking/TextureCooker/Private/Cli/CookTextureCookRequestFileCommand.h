#pragma once

#include "Cli/TextureCookerCommand.h"

#include <filesystem>
#include <string_view>

	class CookTextureCookRequestFileCommand final : public TextureCookerCommand
	{
	  public:
		static bool MatchesName(std::string_view commandName) noexcept;

		int Execute(const std::filesystem::path& requestFilePath) const override;
	};
