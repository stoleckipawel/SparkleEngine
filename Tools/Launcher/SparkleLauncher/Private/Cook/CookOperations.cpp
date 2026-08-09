#include "SparkleLauncher/CookOperations.h"

#include "CookOperationProcessRequests.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <array>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{

	struct CookToolRuntimeReadiness
	{
		bool Ready = true;
		std::vector<std::string> MissingSupportEntries;
	};

	std::vector<std::string> GetRequiredCookToolRuntimeFiles(const std::filesystem::path& toolPath)
	{
		const std::string toolName = toolPath.stem().string();
		if (toolName == "ShaderCompiler")
		{
			return {
			    "dxcompiler.dll",
			    "slang.dll",
			    "slang-compiler.dll",
			    "slang-glsl-module.dll",
			    "slang-glslang.dll",
			    "slang-rt.dll",
			    "slang.slang",
			};
		}

		return {};
	}

	std::vector<std::string> GetRequiredCookToolRuntimeDirectoryPrefixes(const std::filesystem::path& toolPath)
	{
		const std::string toolName = toolPath.stem().string();
		if (toolName == "ShaderCompiler")
		{
			return {"slang-standard-module-"};
		}

		return {};
	}

	bool HasDirectChildDirectoryWithPrefix(const std::filesystem::path& root, std::string_view prefix)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(root, errorCode))
		{
			return false;
		}

		const std::string prefixText(prefix);
		for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(root, errorCode))
		{
			if (errorCode)
			{
				errorCode.clear();
				continue;
			}

			if (!entry.is_directory(errorCode))
			{
				errorCode.clear();
				continue;
			}

			const std::string name = entry.path().filename().string();
			if (name.rfind(prefixText, 0) == 0)
			{
				return true;
			}
			errorCode.clear();
		}

		return false;
	}

	CookToolRuntimeReadiness InspectCookToolRuntimeReadiness(const std::filesystem::path& toolPath)
	{
		CookToolRuntimeReadiness readiness;
		const std::filesystem::path toolDirectory = toolPath.parent_path();
		std::error_code errorCode;
		for (const std::string& runtimeFileName : GetRequiredCookToolRuntimeFiles(toolPath))
		{
			errorCode.clear();
			if (std::filesystem::exists(toolDirectory / runtimeFileName, errorCode) && !errorCode)
			{
				continue;
			}

			readiness.Ready = false;
			readiness.MissingSupportEntries.push_back(runtimeFileName);
		}

		for (const std::string& runtimeDirectoryPrefix : GetRequiredCookToolRuntimeDirectoryPrefixes(toolPath))
		{
			errorCode.clear();
			if (HasDirectChildDirectoryWithPrefix(toolDirectory, runtimeDirectoryPrefix))
			{
				continue;
			}

			readiness.Ready = false;
			readiness.MissingSupportEntries.push_back(runtimeDirectoryPrefix + "*");
		}

		return readiness;
	}

	static void AddReadiness(CookOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(CookOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static std::string ResolveCookToolProfile(std::string_view runtimeProfileName)
	{
		const std::optional<BuildProfile> runtimeProfile = FindBuildProfile(runtimeProfileName);
		if (!runtimeProfile.has_value())
		{
			return "DevelopmentEditor";
		}

		for (const BuildProfile& profile : GetBuildProfileCatalog())
		{
			if (profile.State == runtimeProfile->State && profile.Target == BuildProfileTarget::Editor)
			{
				return profile.Name;
			}
		}

		return "DevelopmentEditor";
	}

	static std::vector<std::string> GetRequiredCookToolNames(const CookOperationPlan& plan)
	{
		switch (plan.Kind)
		{
			case CookOperationKind::CookShaders:
#if SPARKLE_ENABLE_SHADER_COMPILER
				return {"ShaderCompiler"};
#else
				return {};
#endif
			case CookOperationKind::BuildTextures:
#if SPARKLE_ENABLE_CONTENT_PIPELINE
				return {"AssetCooker", "TextureCooker"};
#else
				return {};
#endif
			case CookOperationKind::BuildSceneAssets:
#if SPARKLE_ENABLE_CONTENT_PIPELINE
				return {"AssetCooker"};
#else
				return {};
#endif
			case CookOperationKind::CookAllAssets:
			{
				std::vector<std::string> tools;
#if SPARKLE_ENABLE_CONTENT_PIPELINE
				tools.push_back("AssetCooker");
				tools.push_back("TextureCooker");
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
				tools.push_back("ShaderCompiler");
#endif
				return tools;
			}
		}

		return {};
	}

	static void PopulateCookEffects(CookOperationPlan& plan)
	{
		if (plan.Request.Mode == CookMode::Force)
		{
			AddPlannedEffect(plan, "Remove cooked outputs before cooking: " + plan.CookedOutputDirectory.string());
		}

		switch (plan.Kind)
		{
			case CookOperationKind::CookShaders:
				AddPlannedEffect(plan, "Shader backend: " + plan.Request.ShaderBackend + ".");
				if (!plan.Request.ShaderTargets.empty())
				{
					std::vector<std::string_view> shaderTargets;
					for (const std::string& target : plan.Request.ShaderTargets)
					{
						shaderTargets.push_back(target);
					}
					AddPlannedEffect(plan, "Shader targets: " + Strings::Join(shaderTargets, ", ") + ".");
				}
				AddPlannedEffect(plan, std::string("Shader cache: ") + (plan.Request.ShaderUseCache ? "enabled." : "disabled."));
				AddPlannedEffect(
				    plan,
				    std::string("Shader debug info: ") + (plan.Request.ShaderEnableDebugInfo ? "enabled." : "disabled."));
				AddPlannedEffect(
				    plan,
				    std::string("Shader optimizations: ") + (plan.Request.ShaderEnableOptimizations ? "enabled." : "disabled."));
				AddPlannedEffect(
				    plan,
				    std::string("Warnings as errors: ") + (plan.Request.ShaderWarningsAsErrors ? "enabled." : "disabled."));
				AddPlannedEffect(plan, std::string("Strip debug info: ") + (plan.Request.ShaderStripDebugInfo ? "enabled." : "disabled."));
				if (!plan.Request.ShaderCacheDirectory.empty())
				{
					AddPlannedEffect(plan, "Override shader cache directory: " + plan.Request.ShaderCacheDirectory.string() + ".");
				}
				if (plan.Request.ShaderPackages.empty())
				{
					AddPlannedEffect(plan, "Cook shader packages for " + plan.Request.ContentId + ".");
				}
				else
				{
					std::vector<std::string_view> packages;
					for (const std::string& packageId : plan.Request.ShaderPackages)
					{
						packages.push_back(packageId);
					}
					AddPlannedEffect(plan, "Cook selected shader packages: " + Strings::Join(packages, ", ") + ".");
				}
				return;
			case CookOperationKind::BuildTextures:
				AddPlannedEffect(plan, "Cook texture assets for " + plan.Request.ContentId + ".");
				return;
			case CookOperationKind::BuildSceneAssets:
				AddPlannedEffect(plan, "Cook scene, mesh, and material assets for " + plan.Request.ContentId + ".");
				return;
			case CookOperationKind::CookAllAssets:
				AddPlannedEffect(plan, "Shader phase backend: " + plan.Request.ShaderBackend + ".");
				if (!plan.Request.ShaderTargets.empty())
				{
					std::vector<std::string_view> shaderTargets;
					for (const std::string& target : plan.Request.ShaderTargets)
					{
						shaderTargets.push_back(target);
					}
					AddPlannedEffect(plan, "Shader phase targets: " + Strings::Join(shaderTargets, ", ") + ".");
				}
				AddPlannedEffect(
				    plan,
				    std::string("Shader phase debug info: ") + (plan.Request.ShaderEnableDebugInfo ? "enabled." : "disabled."));
				AddPlannedEffect(
				    plan,
				    std::string("Shader phase optimizations: ") + (plan.Request.ShaderEnableOptimizations ? "enabled." : "disabled."));
				AddPlannedEffect(plan, "Cook all selected level assets.");
				return;
		}
	}

	static void PopulateCookSteps(CookOperationPlan& plan)
	{
		const std::vector<CookOperationProcessStep> processSteps = BuildCookProcessStepsForPlan(plan);
		for (const CookOperationProcessStep& processStep : processSteps)
		{
			CookOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.Destructive = processStep.DeletesCookedOutputs;
			step.DestructivePath = processStep.DestructivePath;
			if (processStep.HasProcessRequest)
			{
				step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
				step.LogPath = processStep.Request.LogPath;
			}
			else
			{
				step.DisplayCommandLine = "Delete " + processStep.DestructivePath.string();
			}
			plan.Steps.push_back(std::move(step));
		}
	}

	std::string ToString(CookOperationKind kind)
	{
		switch (kind)
		{
			case CookOperationKind::CookShaders:
				return "CookShaders";
			case CookOperationKind::BuildTextures:
				return "BuildTextures";
			case CookOperationKind::BuildSceneAssets:
				return "BuildSceneAssets";
			case CookOperationKind::CookAllAssets:
				return "CookAllAssets";
		}

		return "Unknown";
	}

	std::string ToString(CookMode mode)
	{
		switch (mode)
		{
			case CookMode::Incremental:
				return "incremental";
			case CookMode::Force:
				return "force";
		}

		return "unknown";
	}

	const std::vector<CookOperationDefinition>& GetCookOperationDefinitions()
	{
		static const std::vector<CookOperationDefinition> definitions = {
		    {CookOperationKind::CookAllAssets, "cook.all", "Cook", "Cook All", "Prepare all selected level assets."},
		    {CookOperationKind::CookShaders, "cook.shaders", "Cook", "Cook Shaders", "Validate and prepare shader packages."},
		    {CookOperationKind::BuildTextures, "cook.textures", "Cook", "Cook Textures", "Prepare texture assets for runtime use."},
		    {CookOperationKind::BuildSceneAssets,
		        "cook.assets",
		        "Cook",
		        "Cook Scenes And Meshes",
		        "Prepare scene, mesh, and material assets for runtime use."},
		};
		return definitions;
	}

	std::optional<CookOperationDefinition> FindCookOperationDefinition(std::string_view operationId)
	{
		const std::vector<CookOperationDefinition>& definitions = GetCookOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const CookOperationDefinition& definition) { return definition.Id == operationId; });
		return found == definitions.end() ? std::nullopt : std::optional<CookOperationDefinition>(*found);
	}

	CookOperationPlan PlanCookOperation(std::string_view operationId, const CookOperationRequest& request)
	{
		CookOperationPlan plan;
		const std::optional<CookOperationDefinition> definition = FindCookOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown cook operation");
			plan.Operation.FailureSummary = "Unknown cook operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.ToolProfile = ResolveCookToolProfile(request.RuntimeProfile);
		plan.CookedOutputDirectory = GetCookedProjectDirectory(request.RepositoryRoot, request.ContentId);
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"content", request.ContentId});
		plan.Operation.Inputs.push_back({"runtimeProfile", request.RuntimeProfile});
		plan.Operation.Inputs.push_back({"cookMode", ToString(request.Mode)});
		plan.Operation.Inputs.push_back({"toolProfile", plan.ToolProfile});
		plan.Operation.Inputs.push_back({"shaderBackend", request.ShaderBackend});
		if (!request.ShaderTargets.empty())
		{
			std::vector<std::string_view> shaderTargets;
			for (const std::string& target : request.ShaderTargets)
			{
				shaderTargets.push_back(target);
			}
			plan.Operation.Inputs.push_back({"shaderTargets", Strings::Join(shaderTargets, ", ")});
		}
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");
		if (request.Mode == CookMode::Force)
		{
			plan.Operation.DestructiveScope = OperationDestructiveScope::CookedOutputs;
			plan.Operation.RequiresConfirmation = true;
		}

		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot, WorkspaceIde::VisualStudio);
		plan.Freshness = CheckBuildFilesFreshness(request.RepositoryRoot, plan.Toolchain);
		for (const std::string& toolName : GetRequiredCookToolNames(plan))
		{
			plan.RequiredToolPaths.push_back(ResolveSparkleToolPath(request.RepositoryRoot, plan.ToolProfile, toolName));
		}
		AddReadiness(
		    plan,
		    plan.Toolchain.RequiredToolsAvailable ? "Required toolchain is available." : "Required toolchain is incomplete.");
		AddReadiness(plan, plan.Freshness.Summary);
		if (request.ContentId.empty())
		{
			AddReadiness(plan, "Repository content is unavailable for cooking.");
		}
		if (!plan.Freshness.Current)
		{
			AddReadiness(plan, "Generated build files are not current. Run Generate Build Files first.");
		}
		switch (plan.Kind)
		{
			case CookOperationKind::CookShaders:
#if !SPARKLE_ENABLE_SHADER_COMPILER
				AddReadiness(plan, "ShaderCompiler is disabled in this workspace configuration.");
#endif
				break;
			case CookOperationKind::BuildTextures:
			case CookOperationKind::BuildSceneAssets:
#if !SPARKLE_ENABLE_CONTENT_PIPELINE
				AddReadiness(plan, "Content pipeline tools are disabled in this workspace configuration.");
#endif
				break;
			case CookOperationKind::CookAllAssets:
#if !SPARKLE_ENABLE_CONTENT_PIPELINE && !SPARKLE_ENABLE_SHADER_COMPILER
				AddReadiness(plan, "No cook features are enabled in this workspace configuration.");
#endif
				break;
		}
		bool requiredCookToolsAvailable = true;
		for (const std::filesystem::path& toolPath : plan.RequiredToolPaths)
		{
			std::error_code errorCode;
			const bool toolExists = std::filesystem::exists(toolPath, errorCode);
			if (!toolExists || errorCode)
			{
				requiredCookToolsAvailable = false;
				AddReadiness(plan, "Cook tool is missing; run Build Cooking Tools first: " + toolPath.string());
				continue;
			}

			const CookToolRuntimeReadiness runtimeReadiness = InspectCookToolRuntimeReadiness(toolPath);
			requiredCookToolsAvailable = requiredCookToolsAvailable && runtimeReadiness.Ready;
			if (runtimeReadiness.Ready)
			{
				AddReadiness(plan, "Cook tool is ready: " + toolPath.string());
				continue;
			}

			std::vector<std::string_view> missingSupportEntryViews;
			missingSupportEntryViews.reserve(runtimeReadiness.MissingSupportEntries.size());
			for (const std::string& supportEntry : runtimeReadiness.MissingSupportEntries)
			{
				missingSupportEntryViews.push_back(supportEntry);
			}

			AddReadiness(
			    plan,
			    "Cook tool runtime support bundle is incomplete beside " + toolPath.filename().string()
			        + "; run Build Cooking Tools first after Sync shows the Vulkan SDK as ready: "
			        + Strings::Join(missingSupportEntryViews, ", "));
		}
		if (request.Mode == CookMode::Force && !request.ForceRecookConfirmed)
		{
			AddReadiness(plan, "Clean before cooking requires confirmation for: " + plan.CookedOutputDirectory.string());
		}

		PopulateCookEffects(plan);
		if (plan.Toolchain.RequiredToolsAvailable && plan.Freshness.Current && requiredCookToolsAvailable && !request.ContentId.empty())
		{
			PopulateCookSteps(plan);
		}

		plan.CanRun = plan.Toolchain.RequiredToolsAvailable && plan.Freshness.Current && requiredCookToolsAvailable
		    && !request.ContentId.empty() && (request.Mode != CookMode::Force || request.ForceRecookConfirmed);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const CookOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			if (!step.LogPath.empty())
			{
				dryRun << "\n    Log: " << step.LogPath.string();
			}
		}
		if (plan.Steps.empty())
		{
			dryRun << "\n  No command step available until readiness issues are resolved.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}
