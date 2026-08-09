#include "CookOperationProcessRequests.h"

#include "SparkleLauncher/CookOperations.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher::CookToolDependencyContractTests
{
	static void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	static bool ContainsArgument(const std::vector<std::string>& arguments, std::string_view expected)
	{
		return std::ranges::find(arguments, expected) != arguments.end();
	}

	static std::filesystem::path ToolPath(const std::filesystem::path& repositoryRoot, std::string_view toolName)
	{
		return repositoryRoot / "artifacts" / "dev" / "tools" / toolName / "DevelopmentEditor" / (std::string(toolName) + ".exe");
	}

	static CookOperationPlan Plan(std::string_view operationId)
	{
		CookOperationRequest request;
		request.RepositoryRoot = "C:/Sparkle";
		request.ContentId = "Showcase";
		request.RuntimeProfile = "DevelopmentGame";
		return PlanCookOperation(operationId, request);
	}

	static void RequireToolPaths(std::string_view operationId, const std::vector<std::filesystem::path>& expected)
	{
		const CookOperationPlan plan = Plan(operationId);

		Require(plan.ToolProfile == "DevelopmentEditor", "DevelopmentGame did not select DevelopmentEditor host tools.");
		Require(plan.RequiredToolPaths == expected, "A cook operation did not declare every executable in its production path.");
	}

	static void EveryCookOperationDeclaresItsCompleteHostToolSet()
	{
		const std::filesystem::path repositoryRoot = "C:/Sparkle";
		std::vector<std::filesystem::path> shaderTools;
		std::vector<std::filesystem::path> textureTools;
		std::vector<std::filesystem::path> sceneTools;
		std::vector<std::filesystem::path> allTools;
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		textureTools.push_back(ToolPath(repositoryRoot, "AssetCooker"));
		textureTools.push_back(ToolPath(repositoryRoot, "TextureCooker"));
		sceneTools.push_back(ToolPath(repositoryRoot, "AssetCooker"));
		allTools.push_back(ToolPath(repositoryRoot, "AssetCooker"));
		allTools.push_back(ToolPath(repositoryRoot, "TextureCooker"));
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
		shaderTools.push_back(ToolPath(repositoryRoot, "ShaderCompiler"));
		allTools.push_back(ToolPath(repositoryRoot, "ShaderCompiler"));
#endif
		RequireToolPaths("cook.shaders", shaderTools);
		RequireToolPaths("cook.textures", textureTools);
		RequireToolPaths("cook.assets", sceneTools);
		RequireToolPaths("cook.all", allTools);
	}

	static void AssetCookerReceivesTheProvenHostToolProfile()
	{
		CookOperationPlan plan;
		plan.Kind = CookOperationKind::CookAllAssets;
		plan.RepositoryRoot = "C:/Sparkle";
		plan.Request.ContentId = "Showcase";
		plan.Request.RuntimeProfile = "DevelopmentGame";
		plan.ToolProfile = "DevelopmentEditor";
		plan.Operation.Id = "cook.all";
		plan.Toolchain.RequiredToolsAvailable = true;
		plan.Freshness.Current = true;

		const std::vector<CookOperationProcessStep> steps = BuildCookProcessStepsForPlan(plan);
		std::size_t expectedStepCount = 0;
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		expectedStepCount += 2;
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
		expectedStepCount += 1;
#endif
		Require(steps.size() == expectedStepCount, "Cook All did not retain every enabled cooking stage.");
		for (const CookOperationProcessStep& step : steps)
		{
			if (step.Id != "cook-textures" && step.Id != "cook-scene-assets")
			{
				continue;
			}

			Require(
			    step.Request.ExecutablePath == plan.RepositoryRoot / "artifacts/dev/tools/AssetCooker/DevelopmentEditor/AssetCooker.exe",
			    "An AssetCooker stage did not use the proven editor-host artifact.");
			Require(ContainsArgument(step.Request.Arguments, "DevelopmentGame"), "AssetCooker lost the runtime output profile.");
			const auto profileOption = std::ranges::find(step.Request.Arguments, "--tool-profile");
			Require(profileOption != step.Request.Arguments.end(), "AssetCooker was not given the proven host-tool profile.");
			Require(
			    std::next(profileOption) != step.Request.Arguments.end() && *std::next(profileOption) == "DevelopmentEditor",
			    "AssetCooker received a different host-tool profile than readiness proved.");
		}
	}
}

int main()
{
	using namespace SparkleLauncher::CookToolDependencyContractTests;
	try
	{
		EveryCookOperationDeclaresItsCompleteHostToolSet();
		AssetCookerReceivesTheProvenHostToolProfile();
		std::cout << "[PASS] Launcher cook-tool dependency contract\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "[FAIL] Launcher cook-tool dependency contract: " << error.what() << '\n';
		return 1;
	}
}
