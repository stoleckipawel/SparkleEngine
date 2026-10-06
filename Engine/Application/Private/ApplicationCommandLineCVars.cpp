#include "PCH.h"

#include "ApplicationCommandLineCVars.h"

#include "Core/Public/Console/CVar.h"
#include "Core/Public/Console/CVarRegistry.h"
#include "Core/Public/Strings/StringUtils.h"
#include "Core/Public/Process/CommandLineUtils.h"

#include <string>
#include <string_view>

namespace ApplicationCommandLineCVars
{

	static void ApplyAssignment(std::string_view assignment)
	{
		const std::size_t separator = assignment.find('=');
		if (separator == std::string_view::npos || separator == 0 || separator + 1 >= assignment.size())
		{
			return;
		}

		const std::string name(assignment.substr(0, separator));
		const std::string value(assignment.substr(separator + 1));
		ConsoleVariableBase* variable = ConsoleVariableRegistry::Get().Find(name);
		if (variable == nullptr)
		{
			return;
		}

		std::string errorMessage;
		variable->TrySetValueFromString(value, errorMessage);
	}

	void Apply()
	{
		std::wstring_view commandLine{GetCommandLineW()};
		std::size_t offset = 0;
		while (offset < commandLine.size())
		{
			const std::wstring_view wideToken = CommandLine::ReadToken(commandLine, offset);
			if (wideToken.empty())
			{
				continue;
			}

			const std::string token = Strings::ToNarrow(wideToken);
			constexpr std::string_view cvarEqualsPrefix = "--cvar=";
			if (Strings::StartsWithIgnoreCase(token, cvarEqualsPrefix))
			{
				ApplyAssignment(std::string_view(token).substr(cvarEqualsPrefix.size()));
				continue;
			}

			if (Strings::EqualsIgnoreCase(token, "--cvar") || Strings::EqualsIgnoreCase(token, "--set-cvar"))
			{
				const std::wstring_view wideValue = CommandLine::ReadToken(commandLine, offset);
				ApplyAssignment(Strings::ToNarrow(wideValue));
			}
		}
	}
}
