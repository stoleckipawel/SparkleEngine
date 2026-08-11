#include "CookOperationProcessRequests.h"

#include "SparkleLauncher/CookOperations.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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

	static CookOperationPlan PlanWorkspace(std::vector<CookWorkspaceScope> scopes)
	{
		CookOperationRequest request;
		request.RepositoryRoot = "C:/Sparkle";
		request.ContentId = "Showcase";
		request.RuntimeProfile = "DevelopmentGame";
		request.SelectedScopes = std::move(scopes);
		return PlanCookOperation("cook.workspace", request);
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

		std::vector<std::filesystem::path> selectedTools;
#if SPARKLE_ENABLE_SHADER_COMPILER
		selectedTools.push_back(ToolPath(repositoryRoot, "ShaderCompiler"));
#endif
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		selectedTools.push_back(ToolPath(repositoryRoot, "AssetCooker"));
		selectedTools.push_back(ToolPath(repositoryRoot, "TextureCooker"));
#endif
		const CookOperationPlan selectedPlan =
		    PlanWorkspace({CookWorkspaceScope::Shaders, CookWorkspaceScope::Textures, CookWorkspaceScope::Shaders});
		Require(selectedPlan.RequiredToolPaths == selectedTools, "Cook Workspace did not deduplicate the selected scopes' host tools.");
	}

	static void CookWorkspaceRunsOnlySelectedStages()
	{
		CookOperationPlan plan;
		plan.Kind = CookOperationKind::CookWorkspace;
		plan.RepositoryRoot = "C:/Sparkle";
		plan.Request.ContentId = "Showcase";
		plan.Request.RuntimeProfile = "DevelopmentGame";
		plan.Request.SelectedScopes = {CookWorkspaceScope::Shaders, CookWorkspaceScope::SceneAssets};
		plan.ToolProfile = "DevelopmentEditor";
		plan.Operation.Id = "cook.workspace";
		plan.Toolchain.RequiredToolsAvailable = true;
		plan.Freshness.Current = true;

		const std::vector<CookOperationProcessStep> steps = BuildCookProcessStepsForPlan(plan);
		std::size_t expectedStepCount = 0;
#if SPARKLE_ENABLE_SHADER_COMPILER
		expectedStepCount += 1;
#endif
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		expectedStepCount += 1;
#endif
		Require(steps.size() == expectedStepCount, "Cook Workspace did not retain exactly the selected cooking stages.");
		Require(
		    std::ranges::none_of(steps, [](const CookOperationProcessStep& step) { return step.Id == "cook-textures"; }),
		    "Cook Workspace scheduled an unselected texture stage.");

		const CookOperationPlan emptyPlan = PlanWorkspace({});
		Require(
		    std::ranges::any_of(
		        emptyPlan.ReadinessMessages,
		        [](const std::string& message) { return message.find("Select at least one output") != std::string::npos; }),
		    "Cook Workspace did not explain why an empty output selection is blocked.");
	}

	static void LauncherShaderCookUsesCompilerOwnedSelection()
	{
#if SPARKLE_ENABLE_SHADER_COMPILER
		CookOperationPlan plan;
		plan.Kind = CookOperationKind::CookWorkspace;
		plan.RepositoryRoot = "C:/Sparkle";
		plan.Request.ContentId = "Showcase";
		plan.Request.RuntimeProfile = "DevelopmentGame";
		plan.Request.SelectedScopes = {CookWorkspaceScope::Shaders};
		plan.ToolProfile = "DevelopmentEditor";
		plan.Operation.Id = "cook.workspace";

		const std::vector<CookOperationProcessStep> steps = BuildCookProcessStepsForPlan(plan);
		Require(steps.size() == 1 && steps.front().Id == "cook-shaders", "Launcher shader cooking did not produce one complete cook step.");
		Require(
		    !steps.front().Request.Arguments.empty() && steps.front().Request.Arguments.front() == "cook",
		    "Launcher shader cooking did not invoke the complete ShaderCompiler cook command.");
		Require(
		    !ContainsArgument(steps.front().Request.Arguments, "--package"),
		    "Launcher shader cooking overrode compiler-owned package discovery.");
		Require(
		    !ContainsArgument(steps.front().Request.Arguments, "--target"),
		    "Launcher shader cooking overrode compiler-owned runtime targets.");
#endif
	}

	static void ExistingCookToolsDoNotRequireBuildWorkspaceReadiness()
	{
#if SPARKLE_ENABLE_CONTENT_PIPELINE
		const std::filesystem::path repositoryRoot = std::filesystem::temp_directory_path()
		    / ("SparkleCookWorkspace-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		const std::filesystem::path assetCookerPath = ToolPath(repositoryRoot, "AssetCooker");
		std::filesystem::create_directories(assetCookerPath.parent_path());
		std::ofstream(assetCookerPath).put('\n');

		CookOperationRequest request;
		request.RepositoryRoot = repositoryRoot;
		request.ContentId = "Showcase";
		request.RuntimeProfile = "DevelopmentGame";
		request.SelectedScopes = {CookWorkspaceScope::SceneAssets};
		const CookOperationPlan plan = PlanCookOperation("cook.workspace", request);
		std::filesystem::remove_all(repositoryRoot);

		Require(!plan.Freshness.Current, "The isolated cook contract unexpectedly found current generated build files.");
		Require(plan.CanRun, "An available cook tool was blocked by unrelated host-build workspace readiness.");
		Require(
		    plan.Steps.size() == 1 && plan.Steps.front().Id == "cook-scene-assets",
		    "The available scene cooker stage was not planned.");
#endif
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
		CookWorkspaceRunsOnlySelectedStages();
		LauncherShaderCookUsesCompilerOwnedSelection();
		ExistingCookToolsDoNotRequireBuildWorkspaceReadiness();
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
