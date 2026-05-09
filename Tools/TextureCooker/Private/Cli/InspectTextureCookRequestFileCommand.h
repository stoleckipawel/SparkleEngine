#pragma once

#include "Cli/TextureCookerCommand.h"

#include "TextureCookRequestList.h"

#include <cstddef>
#include <filesystem>
#include <string_view>

	class InspectTextureCookRequestFileCommand final : public TextureCookerCommand
	{
	  public:
		static bool MatchesName(std::string_view commandName) noexcept;

		int Execute(const std::filesystem::path& requestFilePath) const override;

	  private:
		static void PrintRequest(const TextureCookRequest& request);
		static void PrintSummary(const std::filesystem::path& requestFilePath, std::size_t requestCount);
	};