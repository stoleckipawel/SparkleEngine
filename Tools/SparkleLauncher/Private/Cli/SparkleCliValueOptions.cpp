#include "SparkleCliParser.h"

#include <ostream>

namespace SparkleLauncher
{
	bool SparkleCliParser::TryAssignStringValue(std::string_view argument, const char* value, SparkleCliArguments& outArguments, bool& outHandled) const
	{
		if (argument == "--root")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.StartPath = value;
			}
			return true;
		}
		if (argument == "--project")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.ProjectId = value;
			}
			return true;
		}
		if (argument == "--editor-profile")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.EditorProfile = value;
			}
			return true;
		}
		if (argument == "--runtime-profile")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.RuntimeProfile = value;
			}
			return true;
		}
		if (argument == "--target")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.SelectedTargets.push_back(value);
			}
			return true;
		}
		if (argument == "--shader-package")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.ShaderPackages.push_back(value);
			}
			return true;
		}
		if (argument == "--validation-target")
		{
			outHandled = true;
			if (value != nullptr)
			{
				outArguments.ValidationTargets.push_back(value);
			}
			return true;
		}
		return false;
	}

	bool SparkleCliParser::TryAssignModeValue(std::string_view argument, const char* value, SparkleCliArguments& outArguments, std::ostream& error, bool& outHandled) const
	{
		if (argument == "--format-mode")
		{
			outHandled = true;
			FormatMode mode = FormatMode::Check;
			if (value == nullptr || !TryParseFormatMode(value, mode))
			{
				error << "Sparkle: --format-mode expects check or apply.\n";
				return false;
			}
			outArguments.RequestedFormatMode = mode;
			return true;
		}
		if (argument == "--clean-scope")
		{
			outHandled = true;
			CleanScope scope = CleanScope::SelectedProjectCookedOutputs;
			if (value == nullptr || !TryParseCleanScope(value, scope))
			{
				error << "Sparkle: unsupported clean scope.\n";
				return false;
			}
			outArguments.RequestedCleanScope = scope;
			return true;
		}
		return false;
	}

	bool SparkleCliParser::IsValueOption(std::string_view argument) const
	{
		return argument == "--root" || argument == "--project" || argument == "--editor-profile" || argument == "--runtime-profile" ||
		       argument == "--target" || argument == "--shader-package" || argument == "--validation-target" || argument == "--format-mode" || argument == "--clean-scope";
	}

	bool SparkleCliParser::TryParseFormatMode(std::string_view text, FormatMode& outMode) const
	{
		if (text == "check")
		{
			outMode = FormatMode::Check;
			return true;
		}
		if (text == "apply")
		{
			outMode = FormatMode::Apply;
			return true;
		}
		return false;
	}

	bool SparkleCliParser::TryParseCleanScope(std::string_view text, CleanScope& outScope) const
	{
		if (text == "selected-cooked" || text == "selected-project-cooked-outputs")
		{
			outScope = CleanScope::SelectedProjectCookedOutputs;
			return true;
		}
		if (text == "all-cooked" || text == "all-cooked-outputs")
		{
			outScope = CleanScope::AllCookedOutputs;
			return true;
		}
		if (text == "build-tree")
		{
			outScope = CleanScope::BuildTree;
			return true;
		}
		if (text == "shader-cache")
		{
			outScope = CleanScope::ShaderCache;
			return true;
		}
		if (text == "deps" || text == "third-party-dependency-cache")
		{
			outScope = CleanScope::ThirdPartyDependencyCache;
			return true;
		}
		if (text == "logs")
		{
			outScope = CleanScope::Logs;
			return true;
		}
		if (text == "pristine" || text == "pristine-generated-workspace")
		{
			outScope = CleanScope::PristineGeneratedWorkspace;
			return true;
		}
		return false;
	}
}