#include "CookOperationProcessRequests.h"

#include "CMakeWorkflowProcessRequests.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <utility>

namespace SparkleLauncher
{
	static void AppendCommonShaderCompilerArguments(const CookOperationPlan& plan, std::vector<std::string>& arguments)
	{
		if (!plan.Request.ShaderUseCache)
		{
			arguments.push_back("--no-cache");
		}
		if (!plan.Request.ShaderCacheDirectory.empty())
		{
			arguments.push_back("--cache-dir");
			arguments.push_back(plan.Request.ShaderCacheDirectory.string());
		}
		for (const std::string& target : plan.Request.ShaderTargets)
		{
			arguments.push_back("--target");
			arguments.push_back(target);
		}
		if (!plan.Request.ShaderBackend.empty())
		{
			arguments.push_back("--backend");
			arguments.push_back(plan.Request.ShaderBackend);
		}
		if (plan.Request.ShaderEnableDebugInfo)
		{
			arguments.push_back("--debug-info");
		}
		if (!plan.Request.ShaderEnableOptimizations)
		{
			arguments.push_back("--disable-optimizations");
		}
		arguments.push_back("--warnings-as-errors");
		arguments.push_back(plan.Request.ShaderWarningsAsErrors ? "on" : "off");
		arguments.push_back("--strip-debug");
		arguments.push_back(plan.Request.ShaderStripDebugInfo ? "on" : "off");
		if (!plan.Request.ShaderDebugArtifactDirectory.empty())
		{
			arguments.push_back("--debug-artifacts");
			arguments.push_back(plan.Request.ShaderDebugArtifactDirectory.string());
		}
	}

	static ProcessRequest MakeAssetCookerRequest(const CookOperationPlan& plan, std::string_view command, std::string_view logFileName)
	{
		ProcessRequest process;
		process.ExecutablePath = ResolveSparkleToolPath(plan.RepositoryRoot, plan.ToolProfile, "AssetCooker");
		process.WorkingDirectory = plan.RepositoryRoot;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, logFileName);
		process.Arguments = {std::string(command), plan.Request.ProjectId, plan.Request.RuntimeProfile, "--root", plan.RepositoryRoot.string()};
		return process;
	}

	static ProcessRequest MakeShaderCompilerCookAllRequest(const CookOperationPlan& plan)
	{
		ProcessRequest process;
		process.ExecutablePath = ResolveSparkleToolPath(plan.RepositoryRoot, plan.ToolProfile, "ShaderCompiler");
		process.WorkingDirectory = plan.RepositoryRoot / "Projects" / plan.Request.ProjectId;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, "CookShaders.txt");
		process.Arguments = {"cook"};
		AppendCommonShaderCompilerArguments(plan, process.Arguments);
		return process;
	}

	static ProcessRequest MakeShaderCompilerCookRequest(const CookOperationPlan& plan, std::string_view packageId)
	{
		ProcessRequest process;
		process.ExecutablePath = ResolveSparkleToolPath(plan.RepositoryRoot, plan.ToolProfile, "ShaderCompiler");
		process.WorkingDirectory = plan.RepositoryRoot / "Projects" / plan.Request.ProjectId;
		process.LogPath = GetLauncherOperationLogPath(plan.RepositoryRoot, plan.Operation.Id, std::string("ShaderPackage-") + std::string(packageId) + ".txt");
		process.Arguments = {"cook", "--package", std::string(packageId)};
		AppendCommonShaderCompilerArguments(plan, process.Arguments);
		return process;
	}

	static void AddStep(std::vector<CookOperationProcessStep>& steps, std::string id, std::string displayName, ProcessRequest request)
	{
		CookOperationProcessStep step;
		step.Id = std::move(id);
		step.DisplayName = std::move(displayName);
		step.Request = std::move(request);
		steps.push_back(std::move(step));
	}

	static void AddCleanStep(std::vector<CookOperationProcessStep>& steps, const CookOperationPlan& plan)
	{
		CookOperationProcessStep step;
		step.Id = "clean-cooked-output";
		step.DisplayName = "Clean cooked output scope";
		step.DestructivePath = plan.CookedOutputDirectory;
		step.HasProcessRequest = false;
		step.DeletesCookedOutputs = true;
		steps.push_back(std::move(step));
	}

	std::vector<CookOperationProcessStep> BuildCookProcessStepsForPlan(const CookOperationPlan& plan)
	{
		std::vector<CookOperationProcessStep> steps;
		if (!plan.Toolchain.RequiredToolsAvailable || !plan.Freshness.Current)
		{
			return steps;
		}

		if (plan.Request.Mode == CookMode::Force)
		{
			AddCleanStep(steps, plan);
		}

		switch (plan.Kind)
		{
		case CookOperationKind::CookShaders:
#if SPARKLE_ENABLE_SHADER_COMPILER
			if (plan.Request.ShaderPackages.empty())
			{
				AddStep(steps, "cook-shaders", "Cook shader packages", MakeShaderCompilerCookAllRequest(plan));
			}
			else
			{
				for (const std::string& packageId : plan.Request.ShaderPackages)
				{
					AddStep(steps, "cook-shader-package", "Cook shader package " + packageId, MakeShaderCompilerCookRequest(plan, packageId));
				}
			}
#endif
			return steps;
		case CookOperationKind::BuildTextures:
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddStep(steps, "cook-textures", "Cook textures", MakeAssetCookerRequest(plan, "cook-textures", "CookTextures.txt"));
#endif
			return steps;
		case CookOperationKind::BuildSceneAssets:
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddStep(steps, "cook-scene-assets", "Cook meshes", MakeAssetCookerRequest(plan, "cook-assets", "CookSceneAssets.txt"));
#endif
			return steps;
		case CookOperationKind::CookAllAssets:
#if SPARKLE_ENABLE_SHADER_COMPILER
			AddStep(steps, "cook-shaders", "Cook shader packages", MakeShaderCompilerCookAllRequest(plan));
#endif
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddStep(steps, "cook-textures", "Cook textures", MakeAssetCookerRequest(plan, "cook-textures", "CookTextures.txt"));
			AddStep(steps, "cook-scene-assets", "Cook meshes", MakeAssetCookerRequest(plan, "cook-assets", "CookSceneAssets.txt"));
#endif
			return steps;
		}

		return steps;
	}
}
