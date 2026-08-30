#pragma once

#include "Core/Public/CoreAPI.h"

#include <string>
#include <string_view>
#include <vector>

struct SPARKLE_CORE_API ConsoleParsedInput final
{
	bool Succeeded = true;
	std::string CommandName;
	std::vector<std::string> Arguments;
	std::string ErrorMessage;

	bool IsEmpty() const noexcept { return Succeeded && CommandName.empty(); }
};

class SPARKLE_CORE_API ConsoleInputParser final
{
public:
	static ConsoleParsedInput Parse(std::string_view input);

private:
	static ConsoleParsedInput Fail(std::string errorMessage);
};
