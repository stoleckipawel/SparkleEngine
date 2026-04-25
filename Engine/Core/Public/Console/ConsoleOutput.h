#pragma once

#include "Core/Public/Console/ConsoleCommandRegistry.h"
#include "Core/Public/CoreAPI.h"

#include <string>

struct SPARKLE_CORE_API ConsoleOutputRecord final
{
	ConsoleCommandSeverity Severity = ConsoleCommandSeverity::Info;
	std::string Text;
};

class SPARKLE_CORE_API ConsoleOutputSink
{
  public:
	virtual ~ConsoleOutputSink() = default;
	virtual void Append(ConsoleOutputRecord record) = 0;
};
