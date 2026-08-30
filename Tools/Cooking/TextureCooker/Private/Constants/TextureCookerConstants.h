#pragma once

#include <cstdint>
#include <string_view>

class TextureCookerConstants final
{
public:
	static constexpr std::string_view ToolName = "TextureCooker";

	static constexpr int ExitSuccess = 0;
	static constexpr int ExitUsageError = 1;
	static constexpr int ExitComInitializationFailed = 4;
	static constexpr int ExitInspectRequestFileFailed = 5;
	static constexpr int ExitLoadRequestFileFailed = 6;
	static constexpr int ExitCookFailed = 7;

	static constexpr std::string_view InspectRequestFileCommand = "inspect-request-file";
	static constexpr std::string_view CookRequestFileCommand = "cook-request-file";
};
