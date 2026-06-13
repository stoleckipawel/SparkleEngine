#include "AssetCookerCli.h"

#include "../Api/AssetCookerService.h"
#include "../Inspection/AssetCookerSourceInspection.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Diagnostics/Trace.h"
#include "ToolConsole.h"

#include <iostream>
#include <string>
#include <string_view>

struct AssetCookerCliArguments final
{
	std::string command;
	std::string projectName;
	std::string configuration = "DevelopmentGame";
	std::string repositoryRoot;
	std::string sourceScenePath;
	std::string outputPath;
	AssetCookerCategory category = AssetCookerCategory_All;
};

static constexpr std::string_view kAssetCookerProfileUsage =
	"[DebugEditor|DebugGame|DevelopmentEditor|DevelopmentGame|ShippingEditor|ShippingGame]";

static constexpr std::string_view kAssetCookerProfiles[] = {
	"DebugEditor",
	"DebugGame",
	"DevelopmentEditor",
	"DevelopmentGame",
	"ShippingEditor",
	"ShippingGame",
};

static bool AssetCookerCliIsHelp(std::string_view argument)
{
	return argument == "--help" || argument == "-h" || argument == "/?" || argument == "/h";
}

static bool AssetCookerCliIsConfiguration(std::string_view argument)
{
	for (const std::string_view profile : kAssetCookerProfiles)
	{
		if (argument == profile)
		{
			return true;
		}
	}
	return false;
}

static void AssetCookerCliPrintUsage(std::ostream& output)
{
	output << "Usage:\n"
	       << "  AssetCooker cook-project [ProjectName|ALL] " << kAssetCookerProfileUsage << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-shaders <ProjectName> " << kAssetCookerProfileUsage << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-textures <ProjectName> " << kAssetCookerProfileUsage << " [--root <repo-root>]\n"
	       << "  AssetCooker cook-assets <ProjectName> " << kAssetCookerProfileUsage << " [--root <repo-root>]\n"
	       << "  AssetCooker capabilities [--root <repo-root>]\n"
	       << "  AssetCooker recook <ProjectName> <shader|texture|mesh|material|scene|all> " << kAssetCookerProfileUsage << " [--root <repo-root>]\n"
	       << "  AssetCooker inspect-source <source-scene-path>\n"
	       << "  AssetCooker collect-texture-requests <source-scene-path> <request-file-path>\n";
}

static bool AssetCookerCliParseCategory(std::string_view value, AssetCookerCategory& outCategory)
{
	if (value == "all")
	{
		outCategory = AssetCookerCategory_All;
		return true;
	}
	if (value == "shader" || value == "shaders")
	{
		outCategory = AssetCookerCategory_Shaders;
		return true;
	}
	if (value == "texture" || value == "textures")
	{
		outCategory = AssetCookerCategory_Textures;
		return true;
	}
	if (value == "assets" || value == "scene-assets")
	{
		outCategory = AssetCookerCategory_SceneAssets;
		return true;
	}
	if (value == "mesh")
	{
		outCategory = AssetCookerCategory_Mesh;
		return true;
	}
	if (value == "material")
	{
		outCategory = AssetCookerCategory_Material;
		return true;
	}
	if (value == "scene")
	{
		outCategory = AssetCookerCategory_Scene;
		return true;
	}
	return false;
}

static const char* AssetCookerCliGetCategoryName(AssetCookerCategory category) noexcept
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
	case AssetCookerCategory_Texture:
		return "texture";
	case AssetCookerCategory_Shader:
		return "shader";
	case AssetCookerCategory_Mesh:
		return "mesh";
	case AssetCookerCategory_Material:
		return "material";
	case AssetCookerCategory_Scene:
		return "scene";
	default:
		return "unknown";
	}
}

static bool AssetCookerCliParseCommonArguments(
    int argc,
    char** argv,
    int startIndex,
    AssetCookerCliArguments& arguments)
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
			arguments.repositoryRoot = argv[index + 1];
			++index;
			continue;
		}

		if (AssetCookerCliIsConfiguration(argument))
		{
			arguments.configuration = std::string(argument);
			continue;
		}

		if (arguments.projectName.empty())
		{
			arguments.projectName = std::string(argument);
			continue;
		}

		std::cerr << "AssetCooker: unexpected argument '" << argument << "'.\n";
		return false;
	}

	return true;
}

static bool AssetCookerCliParse(int argc, char** argv, AssetCookerCliArguments& arguments)
{
	if (argc < 2 || AssetCookerCliIsHelp(argv[1]))
	{
		return false;
	}

	arguments.command = argv[1];
	if (arguments.command == "cook-project")
	{
		arguments.category = AssetCookerCategory_All;
		return AssetCookerCliParseCommonArguments(argc, argv, 2, arguments);
	}

	if (arguments.command == "cook-shaders")
	{
		arguments.category = AssetCookerCategory_Shaders;
		return AssetCookerCliParseCommonArguments(argc, argv, 2, arguments) && !arguments.projectName.empty();
	}

	if (arguments.command == "cook-textures")
	{
		arguments.category = AssetCookerCategory_Textures;
		return AssetCookerCliParseCommonArguments(argc, argv, 2, arguments) && !arguments.projectName.empty();
	}

	if (arguments.command == "cook-assets")
	{
		arguments.category = AssetCookerCategory_SceneAssets;
		return AssetCookerCliParseCommonArguments(argc, argv, 2, arguments) && !arguments.projectName.empty();
	}

	if (arguments.command == "capabilities")
	{
		return AssetCookerCliParseCommonArguments(argc, argv, 2, arguments);
	}

	if (arguments.command == "recook")
	{
		if (argc < 4)
		{
			return false;
		}
		arguments.projectName = argv[2];
		if (!AssetCookerCliParseCategory(argv[3], arguments.category))
		{
			std::cerr << "AssetCooker: unsupported recook category '" << argv[3] << "'.\n";
			return false;
		}
		return AssetCookerCliParseCommonArguments(argc, argv, 4, arguments);
	}

	if (arguments.command == "inspect-source")
	{
		if (argc != 3)
		{
			return false;
		}
		arguments.sourceScenePath = argv[2];
		return true;
	}

	if (arguments.command == "collect-texture-requests")
	{
		if (argc != 4)
		{
			return false;
		}
		arguments.sourceScenePath = argv[2];
		arguments.outputPath = argv[3];
		return true;
	}

	return false;
}

static void AssetCookerCliPrintResult(const AssetCookerServiceResult& result)
{
	for (const AssetCookerDiagnosticRecord& diagnostic : result.diagnostics)
	{
		const ToolConsoleSeverity severity = diagnostic.severity == AssetCookerDiagnosticSeverity_Error ? ToolConsoleSeverity::Error :
		                                     diagnostic.severity == AssetCookerDiagnosticSeverity_Warning ? ToolConsoleSeverity::Warning :
		                                                                                                     ToolConsoleSeverity::Info;
		if (diagnostic.sourcePath.empty())
		{
			ToolConsole::Message(severity == ToolConsoleSeverity::Error ? std::cerr : std::cout, severity, diagnostic.message);
		}
		else
		{
			ToolConsole::Message(
			    severity == ToolConsoleSeverity::Error ? std::cerr : std::cout,
			    severity,
			    diagnostic.message,
			    {ToolConsole::QuotedField("source", diagnostic.sourcePath)});
		}
	}

	for (const AssetCookerOutputRecord& output : result.outputs)
	{
		ToolConsole::Message(
		    std::cout,
		    ToolConsoleSeverity::Info,
		    "Cooked output",
		    {ToolConsole::Field("type", AssetCookerCliGetCategoryName(output.category)),
		     ToolConsole::QuotedField("name", output.assetId),
		     ToolConsole::QuotedField("path", output.path),
		     ToolConsole::QuotedField("reload", output.reloadHint)});
	}
}

int AssetCookerCli::Run(int argc, char** argv) const
{
	static const auto assetCookerLogger = Logging::GetOrCreateLogger("Tools.AssetCooker");
	SPARKLE_CPU_SCOPE("Tools.AssetCooker.Cli.Run");
	SPARKLE_LOG_SCOPE(assetCookerLogger, spdlog::level::info, "AssetCooker.Cli.Run");

	AssetCookerCliArguments arguments;
	if (!AssetCookerCliParse(argc, argv, arguments))
	{
		AssetCookerCliPrintUsage(std::cerr);
		return 1;
	}

	if (arguments.command == "inspect-source")
	{
		return AssetCookerSourceInspection::InspectSource(arguments.sourceScenePath);
	}

	if (arguments.command == "collect-texture-requests")
	{
		return AssetCookerSourceInspection::CollectTextureRequests(arguments.sourceScenePath, arguments.outputPath);
	}

	AssetCookerConfig config = {};
	config.repositoryRoot = arguments.repositoryRoot.empty() ? nullptr : arguments.repositoryRoot.c_str();
	config.projectName = arguments.projectName.empty() ? nullptr : arguments.projectName.c_str();
	config.configuration = arguments.configuration.c_str();
	SPDLOG_LOGGER_INFO(
	    assetCookerLogger,
	    "AssetCooker command='{}' project='{}' configuration='{}' category={}",
	    arguments.command,
	    arguments.projectName.empty() ? "ALL" : arguments.projectName,
	    arguments.configuration,
	    AssetCookerCliGetCategoryName(arguments.category));

	AssetCookerService service(&config);

	if (arguments.command == "capabilities")
	{
		const AssetCookerCapabilities capabilities = service.QueryCapabilities();
		std::cout << "AssetCooker Capabilities:\n"
		          << "  projectCook=" << capabilities.supportsProjectCook << "\n"
		          << "  selectedRecook=" << capabilities.supportsSelectedRecook << "\n"
		          << "  shaderCook=" << capabilities.supportsShaderCook << "\n"
		          << "  textureCook=" << capabilities.supportsTextureCook << "\n"
		          << "  sceneAssetCook=" << capabilities.supportsSceneAssetCook << "\n"
		          << "  hotReloadOutputs=" << capabilities.supportsHotReloadOutputs << "\n";
		return 0;
	}

	AssetCookerServiceResult result;
	int exitCode = 1;
	if (arguments.command == "recook")
	{
		AssetRecookAsset recookAsset = {};
		recookAsset.category = arguments.category;
		AssetRecookRequest request = {};
		request.projectName = arguments.projectName.c_str();
		request.configuration = arguments.configuration.c_str();
		request.assets = &recookAsset;
		request.assetCount = 1;
		result = service.RecookAssets(&request);
	}
	else
	{
		AssetCookRequest request = {};
		request.category = arguments.category;
		request.projectName = arguments.projectName.empty() ? nullptr : arguments.projectName.c_str();
		request.configuration = arguments.configuration.c_str();
		result = service.CookProject(&request);
	}

	exitCode = result.exitCode;

	AssetCookerCliPrintResult(result);
	SPDLOG_LOGGER_INFO(assetCookerLogger, "AssetCooker command='{}' completed with exitCode={}", arguments.command, exitCode);
	return exitCode;
}
