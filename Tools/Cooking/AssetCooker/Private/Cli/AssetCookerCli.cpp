#include "AssetCookerCli.h"

#include "../Api/AssetCookerService.h"

#include "ToolConsole.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <string>
#include <string_view>

struct AssetCookerCli::Arguments final
{
	std::string Command;
	std::string ProjectName;
	std::string Configuration = "DevelopmentGame";
	std::string ToolProfile;
	std::string RepositoryRoot;
	AssetCookerCategory Category = AssetCookerCategory::All;
};

bool AssetCookerCli::IsHelp(std::string_view argument) noexcept
{
	return argument == "--help" || argument == "-h" || argument == "/?" || argument == "/h";
}

bool AssetCookerCli::IsConfiguration(std::string_view argument) noexcept
{
	static constexpr std::array<std::string_view, 6> Profiles =
	    {"DebugEditor", "DebugGame", "DevelopmentEditor", "DevelopmentGame", "ShippingEditor", "ShippingGame"};

	return std::ranges::find(Profiles, argument) != Profiles.end();
}

bool AssetCookerCli::ParseCommonArguments(int argc, char** argv, int startIndex, Arguments& arguments)
{
	for (int index = startIndex; index < argc; ++index)
	{
		const std::string_view argument(argv[index]);
		if (argument == "--root")
		{
			if (index + 1 >= argc)
			{
				std::cerr << "AssetCooker: --root requires a path.\n";
				return false;
			}

			arguments.RepositoryRoot = argv[++index];
			continue;
		}
		if (argument == "--tool-profile")
		{
			if (index + 1 >= argc)
			{
				std::cerr << "AssetCooker: --tool-profile requires an editor profile.\n";
				return false;
			}

			arguments.ToolProfile = argv[++index];
			continue;
		}

		if (IsConfiguration(argument))
		{
			arguments.Configuration = argument;
			continue;
		}

		if (arguments.ProjectName.empty())
		{
			arguments.ProjectName = argument;
			continue;
		}

		std::cerr << "AssetCooker: unexpected argument '" << argument << "'.\n";
		return false;
	}

	return true;
}

bool AssetCookerCli::Parse(int argc, char** argv, Arguments& arguments)
{
	if (argc < 2 || IsHelp(argv[1]))
	{
		return false;
	}

	arguments.Command = argv[1];
	if (arguments.Command == "cook-project")
	{
		arguments.Category = AssetCookerCategory::All;
		return ParseCommonArguments(argc, argv, 2, arguments);
	}

	if (arguments.Command == "cook-shaders")
	{
		arguments.Category = AssetCookerCategory::Shaders;
	}
	else if (arguments.Command == "cook-textures")
	{
		arguments.Category = AssetCookerCategory::Textures;
	}
	else if (arguments.Command == "cook-assets")
	{
		arguments.Category = AssetCookerCategory::SceneAssets;
	}
	else
	{
		return false;
	}

	return ParseCommonArguments(argc, argv, 2, arguments) && !arguments.ProjectName.empty();
}

void AssetCookerCli::PrintUsage(std::ostream& output)
{
	constexpr std::string_view profile = "[DebugEditor|DebugGame|DevelopmentEditor|DevelopmentGame|ShippingEditor|ShippingGame]";

	output << "Usage:\n"
	       << "  AssetCooker cook-project [ProjectName|ALL] " << profile << " [--tool-profile <EditorProfile>] [--root <repo-root>]\n"
	       << "  AssetCooker cook-shaders <ProjectName> " << profile << " [--tool-profile <EditorProfile>] [--root <repo-root>]\n"
	       << "  AssetCooker cook-textures <ProjectName> " << profile << " [--tool-profile <EditorProfile>] [--root <repo-root>]\n"
	       << "  AssetCooker cook-assets <ProjectName> " << profile << " [--tool-profile <EditorProfile>] [--root <repo-root>]\n";
}

void AssetCookerCli::PrintResult(const AssetCookerServiceResult& result)
{
	for (const AssetCookerDiagnosticRecord& diagnostic : result.diagnostics)
	{
		if (diagnostic.sourcePath.empty())
		{
			ToolConsole::Error(diagnostic.message);
			continue;
		}

		ToolConsole::Message(
		    std::cerr,
		    ToolConsoleSeverity::Error,
		    diagnostic.message,
		    {ToolConsole::QuotedField("source", diagnostic.sourcePath)});
	}

	if (result.exitCode == 0)
	{
		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Cook completed",
		    {ToolConsole::Field("products", std::to_string(result.outputs.size()))});
	}
}

int AssetCookerCli::Run(int argc, char** argv) const
{
	Arguments arguments;
	if (!Parse(argc, argv, arguments))
	{
		PrintUsage(std::cerr);
		return 1;
	}

	AssetCookerService service(
	    arguments.RepositoryRoot.empty() ? nullptr : arguments.RepositoryRoot.c_str(),
	    arguments.ProjectName.empty() ? nullptr : arguments.ProjectName.c_str(),
	    arguments.Configuration.c_str(),
	    arguments.ToolProfile.empty() ? nullptr : arguments.ToolProfile.c_str());

	const AssetCookerServiceResult result = service.Cook(
	    arguments.ProjectName.empty() ? nullptr : arguments.ProjectName.c_str(),
	    arguments.Configuration.c_str(),
	    arguments.Category);

	PrintResult(result);
	return result.exitCode;
}
