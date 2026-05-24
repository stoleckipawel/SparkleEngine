#include "SparkleLauncher/CookOperations.h"

#include "CookOperationProcessRequests.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <utility>

namespace SparkleLauncher
{
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

	static void PopulateCookEffects(CookOperationPlan& plan)
	{
		if (plan.Request.Mode == CookMode::Force)
		{
			AddPlannedEffect(plan, "Clean cooked output scope before cooking: " + plan.CookedOutputDirectory.string());
		}

		switch (plan.Kind)
		{
		case CookOperationKind::CookShaders:
			AddPlannedEffect(plan, "Validate shader registrations with ShaderCompiler.");
			if (plan.Request.ShaderPackages.empty())
			{
				AddPlannedEffect(plan, "Cook shader packages for " + plan.Request.ProjectId + " through AssetCooker.");
			}
			else
			{
				std::vector<std::string_view> packages;
				for (const std::string& packageId : plan.Request.ShaderPackages)
				{
					packages.push_back(packageId);
				}
				AddPlannedEffect(plan, "Cook selected shader package(s): " + Strings::Join(packages, ", ") + ".");
			}
			return;
		case CookOperationKind::BuildTextures:
			AddPlannedEffect(plan, "Cook texture assets for " + plan.Request.ProjectId + " through AssetCooker and TextureCooker.");
			return;
		case CookOperationKind::BuildSceneAssets:
			AddPlannedEffect(plan, "Cook scene, mesh, and material assets for " + plan.Request.ProjectId + " through AssetCooker.");
			return;
		case CookOperationKind::CookAllAssets:
			AddPlannedEffect(plan, "Run full project cook for " + plan.Request.ProjectId + " through AssetCooker.");
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
		    {CookOperationKind::CookAllAssets, "cook.project", "Cook", "Cook All Assets", "Run full incremental or confirmed force cook for the selected project."},
		    {CookOperationKind::CookShaders, "cook.shaders", "Cook", "Cook Shaders", "Validate registrations and cook shader packages."},
		    {CookOperationKind::BuildTextures, "cook.textures", "Cook", "Build Textures", "Cook texture assets through AssetCooker and TextureCooker."},
		    {CookOperationKind::BuildSceneAssets, "cook.assets", "Cook", "Build Meshes", "Cook scene, mesh, and material assets through AssetCooker."},
		};
		return definitions;
	}

	std::optional<CookOperationDefinition> FindCookOperationDefinition(std::string_view operationId)
	{
		const std::vector<CookOperationDefinition>& definitions = GetCookOperationDefinitions();
		const auto found = std::find_if(definitions.begin(), definitions.end(), [operationId](const CookOperationDefinition& definition) {
			return definition.Id == operationId;
		});
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
		plan.CookedOutputDirectory = GetCookedProjectDirectory(request.RepositoryRoot, request.ProjectId);
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"project", request.ProjectId});
		plan.Operation.Inputs.push_back({"runtimeProfile", request.RuntimeProfile});
		plan.Operation.Inputs.push_back({"cookMode", ToString(request.Mode)});
		plan.Operation.Inputs.push_back({"toolProfile", plan.ToolProfile});
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");
		if (request.Mode == CookMode::Force)
		{
			plan.Operation.DestructiveScope = OperationDestructiveScope::SelectedProjectCookedOutputs;
			plan.Operation.RequiresConfirmation = true;
		}

		plan.Toolchain = DetectBuildToolchain(request.RepositoryRoot);
		plan.Freshness = CheckBuildFilesFreshness(request.RepositoryRoot, plan.Toolchain);
		AddReadiness(plan, plan.Toolchain.RequiredToolsAvailable ? "Required toolchain is available." : "Required toolchain is incomplete.");
		AddReadiness(plan, plan.Freshness.Summary);
		if (request.ProjectId.empty())
		{
			AddReadiness(plan, "A project must be selected before cooking.");
		}
		if (request.Mode == CookMode::Force && !request.ForceRecookConfirmed)
		{
			AddReadiness(plan, "Force recook requires confirmation for scope: " + plan.CookedOutputDirectory.string());
		}

		PopulateCookEffects(plan);
		if (plan.Toolchain.RequiredToolsAvailable)
		{
			PopulateCookSteps(plan);
		}

		plan.CanRun = plan.Toolchain.RequiredToolsAvailable && !request.ProjectId.empty() &&
		    (request.Mode != CookMode::Force || request.ForceRecookConfirmed);

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
			dryRun << "\n  No process step available until readiness issues are resolved.";
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}