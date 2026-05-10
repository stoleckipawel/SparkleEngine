#include "AssetCookerCli.h"

#include "../../Public/AssetCookerApi.h"

#include "Core/Public/Diagnostics/Logger.h"
#include "Core/Public/Diagnostics/ScopedLogEvent.h"
#include "Core/Public/Diagnostics/Trace.h"

#include <iostream>
#include <string>
#include <string_view>

struct AssetCookerCliArguments final
{
	std::string command;
	std::string projectName;
	std::string configuration = "Debug";
	std::string repositoryRoot;
	AssetCookerCategory category = AssetCookerCategory_All;
};

static bool AssetCookerCliIsHelp(std::string_view argument)
{
	return argument == "--help" || argument == "-h" || argument == "/?" || argument == "/h";
}

static bool AssetCookerCliIsConfiguration(std::string_view argument)
{
	return argument == "Debug" || argument == "Release" || argument == "RelWithDebInfo";
}

static void AssetCookerCliPrintUsage(std::ostream& output)
{
	output << "Usage:\n"
	       << "  AssetCooker cook-project [ProjectName|ALL] [Debug|Release|RelWithDebInfo] [--root <repo-root>]\n"
	       << "  AssetCooker cook-shaders <ProjectName> [Debug|Release|RelWithDebInfo] [--root <repo-root>]\n"
	       << "  AssetCooker cook-textures <ProjectName> [Debug|Release|RelWithDebInfo] [--root <repo-root>]\n"
	       << "  AssetCooker cook-assets <ProjectName> [Debug|Release|RelWithDebInfo] [--root <repo-root>]\n"
	       << "  AssetCooker capabilities [--root <repo-root>]\n"
	       << "  AssetCooker recook <ProjectName> <shader|texture|mesh|material|scene|all> [Debug|Release|RelWithDebInfo] [--root <repo-root>]\n";
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

	return false;
}

static void AssetCookerCliPrintResult(const AssetCookResult& result)
{
	for (std::uint32_t index = 0; index < result.diagnosticCount; ++index)
	{
		const AssetCookDiagnostic& diagnostic = result.diagnostics[index];
		const char* prefix = diagnostic.severity == AssetCookerDiagnosticSeverity_Error ? "[ERROR]" :
		                     diagnostic.severity == AssetCookerDiagnosticSeverity_Warning ? "[WARN]" :
		                                                                                     "[LOG]";
		std::cout << prefix << " " << (diagnostic.message != nullptr ? diagnostic.message : "") << "\n";
	}

	for (std::uint32_t index = 0; index < result.outputCount; ++index)
	{
		const AssetCookedOutput& output = result.outputs[index];
		std::cout << "[LOG] Output: category=" << AssetCookerCliGetCategoryName(output.category)
		          << " assetId='" << (output.assetId != nullptr ? output.assetId : "") << "'"
		          << " path='" << (output.path != nullptr ? output.path : "") << "'"
		          << " reloadHint='" << (output.reloadHint != nullptr ? output.reloadHint : "") << "'"
		          << " version=" << output.version << "\n";
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

	AssetCookerContext* context = AssetCookerCreateContext(&config);
	if (context == nullptr)
	{
		std::cerr << "AssetCooker: failed to create context.\n";
		return 1;
	}

	if (arguments.command == "capabilities")
	{
		AssetCookerCapabilities capabilities = {};
		const int exitCode = AssetCookerQueryCapabilities(context, &capabilities);
		std::cout << "AssetCooker Capabilities:\n"
		          << "  projectCook=" << capabilities.supportsProjectCook << "\n"
		          << "  selectedRecook=" << capabilities.supportsSelectedRecook << "\n"
		          << "  shaderCook=" << capabilities.supportsShaderCook << "\n"
		          << "  textureCook=" << capabilities.supportsTextureCook << "\n"
		          << "  sceneAssetCook=" << capabilities.supportsSceneAssetCook << "\n"
		          << "  hotReloadOutputs=" << capabilities.supportsHotReloadOutputs << "\n";
		AssetCookerDestroyContext(context);
		return exitCode;
	}

	AssetCookResult result = {};
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
		exitCode = AssetCookerRecookAssets(context, &request, &result);
	}
	else
	{
		AssetCookRequest request = {};
		request.category = arguments.category;
		request.projectName = arguments.projectName.empty() ? nullptr : arguments.projectName.c_str();
		request.configuration = arguments.configuration.c_str();
		exitCode = AssetCookerCookProject(context, &request, &result);
	}

	AssetCookerCliPrintResult(result);
	AssetCookerDestroyContext(context);
	SPDLOG_LOGGER_INFO(assetCookerLogger, "AssetCooker command='{}' completed with exitCode={}", arguments.command, exitCode);
	return exitCode;
}
