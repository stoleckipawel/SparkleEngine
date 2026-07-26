#include "AssetCookerCli.h"

#include "../Api/AssetCookerService.h"

#include "ToolConsole.h"

#include <array>
#include <iostream>
#include <string>
#include <string_view>

struct AssetCookerCli::Arguments final
{
	std::string Command;
	std::string ProjectName;
	std::string Configuration = "DevelopmentGame";
	std::string RepositoryRoot;
	AssetCookerCategory Category = AssetCookerCategory_All;
};

bool AssetCookerCli::IsHelp(std::string_view argument) noexcept
{
	return argument == "--help" || argument == "-h" || argument == "/?" || argument == "/h";
}

bool AssetCookerCli::IsConfiguration(std::string_view argument) noexcept
{
	static constexpr std::array<std::string_view, 6> Profiles = {
	    "DebugEditor",
	    "DebugGame",
	    "DevelopmentEditor",
	    "DevelopmentGame",
	    "ShippingEditor",
	    "ShippingGame"};

	return std::ranges::find(Profiles, argument) != Profiles.end();
}

bool AssetCookerCli::ParseCommonArguments(
    int argc,
    char** argv,
    int startIndex,
    Arguments& arguments)
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
		arguments.Category = AssetCookerCategory_All;
		return ParseCommonArguments(argc, argv, 2, arguments);
	}

	if (arguments.Command == "cook-shaders")
	{
		arguments.Category = AssetCookerCategory_Shaders;
	}
	else if (arguments.Command == "cook-textures")
	{
		arguments.Category = AssetCookerCategory_Textures;
	}
	else if (arguments.Command == "cook-assets")
	{
		arguments.Category = AssetCookerCategory_SceneAssets;
	}
	else
	{
		return false;
	}

	return ParseCommonArguments(argc, argv, 2, arguments) && !arguments.ProjectName.empty();
}

const char* AssetCookerCli::GetCategoryName(AssetCookerCategory category) noexcept
{
	switch (category)
	{
	case AssetCookerCategory_All:
		return "all";
	case AssetCookerCategory_Shaders:
		return "shaders";
	case AssetCookerCategory_Textures:
		return "textures";
	case AssetCookerCategory_SceneAssets:
		return "scene-assets";
	case AssetCookerCategory_Meshes:
		return "meshes";
	case AssetCookerCategory_Materials:
		return "materials";
	default:
		return "unknown";
	}
}

void AssetCookerCli::PrintUsage(std::ostream& output)
{
	constexpr std::string_view profile =
	    "[DebugEditor|DebugGame|DevelopmentEditor|DevelopmentGame|ShippingEditor|ShippingGame]";

	output << "Usage:\n"
	       << "  AssetCooker cook-project [ProjectName|ALL] " << profile << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-shaders <ProjectName> " << profile << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-textures <ProjectName> " << profile << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-assets <ProjectName> " << profile << " [--root <repo-root>]\n";
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

	for (const AssetCookerOutputRecord& output : result.outputs)
	{
		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Cooked output",
		    {ToolConsole::Field("type", GetCategoryName(output.category)),
		     ToolConsole::QuotedField("name", output.assetId),
		     ToolConsole::QuotedField("path", output.path)});
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
	    arguments.Configuration.c_str());

	const AssetCookerServiceResult result = service.Cook(
	    arguments.ProjectName.empty() ? nullptr : arguments.ProjectName.c_str(),
	    arguments.Configuration.c_str(),
	    arguments.Category);

	PrintResult(result);
	return result.exitCode;
}
