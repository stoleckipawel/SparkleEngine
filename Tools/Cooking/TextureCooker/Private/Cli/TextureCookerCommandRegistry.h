#pragma once

#include "Cli/TextureCookerCommand.h"

#include <memory>
#include <ostream>
#include <string_view>

class TextureCookerCommandRegistry final
{
public:
	static std::unique_ptr<TextureCookerCommand> Create(std::string_view commandName);
	static void PrintUsage(std::ostream& output);
};
