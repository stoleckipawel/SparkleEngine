#include "SparkleCliParser.h"

#include <ostream>

namespace SparkleLauncher
{
	bool SparkleCliParser::Parse(int argc, char** argv, SparkleCliArguments& outArguments, std::ostream& error) const
	{
		for (int index = 1; index < argc; ++index)
		{
			const std::string_view argument(argv[index]);
			if (TryHandleFlagOption(argument, outArguments))
			{
				continue;
			}

			bool valueOptionHandled = false;
			if (!TryHandleValueOption(argc, argv, index, argument, outArguments, error, valueOptionHandled))
			{
				return false;
			}
			if (valueOptionHandled)
			{
				continue;
			}

			if (!argument.empty() && argument.front() == '-')
			{
				error << "Sparkle: unexpected argument '" << argument << "'.\n";
				return false;
			}
			if (!AssignOperationId(argument, outArguments, error))
			{
				return false;
			}
		}

		return true;
	}

	bool SparkleCliParser::TryHandleFlagOption(std::string_view argument, SparkleCliArguments& outArguments) const
	{
		if (argument == "--help" || argument == "-h" || argument == "/?")
		{
			outArguments.ShowHelp = true;
			return true;
		}
		if (argument == "--list-operations")
		{
			outArguments.ListOperations = true;
			return true;
		}
		if (argument == "--list-validation-targets")
		{
			outArguments.ListValidationTargets = true;
			return true;
		}
		if (argument == "--dry-run")
		{
			outArguments.DryRun = true;
			return true;
		}
		if (argument == "--force-configure")
		{
			outArguments.ForceConfigure = true;
			return true;
		}
		if (argument == "--force-recook")
		{
			outArguments.RequestedCookMode = CookMode::Force;
			return true;
		}
		if (argument == "--confirm-force-recook")
		{
			outArguments.ForceRecookConfirmed = true;
			return true;
		}
		if (argument == "--confirm-clean")
		{
			outArguments.CleanConfirmed = true;
			return true;
		}
		if (argument == "--smoke-trace")
		{
			outArguments.SmokeTrace = true;
			return true;
		}
		if (argument == "--smoke-skip-level-switching")
		{
			outArguments.SmokeSkipLevelSwitching = true;
			return true;
		}
		return false;
	}

	bool SparkleCliParser::TryHandleValueOption(
	    int argc,
	    char** argv,
	    int& index,
	    std::string_view argument,
	    SparkleCliArguments& outArguments,
	    std::ostream& error,
	    bool& outHandled) const
	{
		outHandled = false;
		if (!IsValueOption(argument))
		{
			return true;
		}

		outHandled = true;
		const char* value = RequireValue(argc, argv, index, argument, error);
		if (value == nullptr)
		{
			return false;
		}
		if (TryAssignStringValue(argument, value, outArguments, outHandled))
		{
			return true;
		}
		return TryAssignModeValue(argument, value, outArguments, error, outHandled);
	}

	bool SparkleCliParser::AssignOperationId(std::string_view argument, SparkleCliArguments& outArguments, std::ostream& error) const
	{
		if (!outArguments.OperationId.empty())
		{
			error << "Sparkle: multiple operation ids were provided.\n";
			return false;
		}
		outArguments.OperationId = argument;
		return true;
	}

	const char* SparkleCliParser::RequireValue(int argc, char** argv, int& index, std::string_view optionName, std::ostream& error) const
	{
		if (index + 1 >= argc)
		{
			error << "Sparkle: " << optionName << " requires a value.\n";
			return nullptr;
		}
		return argv[++index];
	}
}