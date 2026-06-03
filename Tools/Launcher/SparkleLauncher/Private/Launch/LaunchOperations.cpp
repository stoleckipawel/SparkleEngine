#include "SparkleLauncher/LaunchOperations.h"

#include "LaunchOperationProcessRequests.h"
#include "Smoke/RhiSmokeLaunchOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <optional>
#include <sstream>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(LaunchOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void AddPlannedEffect(LaunchOperationPlan& plan, std::string message)
	{
		plan.PlannedEffects.push_back(std::move(message));
	}

	static bool IsRuntimeLaunchTarget(const LaunchOperationRequest& request)
	{
		return request.Target == "runtime";
	}

	static std::string ResolveLaunchProfile(LaunchOperationKind, const LaunchOperationRequest& request)
	{
		return IsRuntimeLaunchTarget(request) ? request.RuntimeProfile : request.EditorProfile;
	}

	static std::optional<BuildProfile> ResolveProfileForLaunch(const LaunchOperationRequest& request, std::string_view profileName)
	{
		const std::optional<BuildProfile> profile = FindBuildProfile(profileName);
		if (!profile.has_value())
		{
			return std::nullopt;
		}

		const BuildProfileTarget expectedTarget = IsRuntimeLaunchTarget(request) ? BuildProfileTarget::Game : BuildProfileTarget::Editor;
		return profile->Target == expectedTarget ? profile : std::nullopt;
	}

	static void PopulateLaunchStep(LaunchOperationPlan& plan)
	{
		if (!plan.CanRun)
		{
			return;
		}

		for (const LaunchOperationProcessStep& processStep : BuildLaunchProcessStepsForPlan(plan))
		{
			LaunchOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
			step.LogPath = processStep.Request.LogPath;
			plan.Steps.push_back(std::move(step));
		}
	}

	static bool DirectoryHasRegularFiles(const std::filesystem::path& directory)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(directory, errorCode))
		{
			return false;
		}

		std::filesystem::recursive_directory_iterator iterator(
		    directory,
		    std::filesystem::directory_options::skip_permission_denied,
		    errorCode);
		const std::filesystem::recursive_directory_iterator end;
		while (iterator != end)
		{
			const std::filesystem::directory_entry entry = *iterator;
			if (entry.is_regular_file(errorCode))
			{
				return true;
			}
			errorCode.clear();
			iterator.increment(errorCode);
			errorCode.clear();
		}

		return false;
	}

	static bool CookedAssetScopeHasFiles(
	    const std::filesystem::path& cookedProjectDirectory,
	    std::string_view relativeDirectory)
	{
		if (DirectoryHasRegularFiles(cookedProjectDirectory / std::string(relativeDirectory)))
		{
			return true;
		}

		const std::filesystem::path cookedSharedDirectory = cookedProjectDirectory.parent_path() / "Shared";
		return DirectoryHasRegularFiles(cookedSharedDirectory / std::string(relativeDirectory));
	}

	static std::filesystem::path FirstExistingOrPreferred(const std::vector<std::filesystem::path>& candidates)
	{
		std::error_code errorCode;
		for (const std::filesystem::path& candidate : candidates)
		{
			if (std::filesystem::exists(candidate, errorCode) && std::filesystem::is_regular_file(candidate, errorCode))
			{
				return candidate;
			}
			errorCode.clear();
		}
		return candidates.empty() ? std::filesystem::path() : candidates.front();
	}

	static std::filesystem::path ResolveLaunchExecutablePath(
	    const LaunchOperationRequest& request,
	    std::string_view profileName,
	    std::string_view targetName)
	{
		std::filesystem::path fileName(targetName);
#if defined(_WIN32)
		if (fileName.extension().empty())
		{
			fileName += ".exe";
		}
#endif
		const std::string productRole = IsRuntimeLaunchTarget(request) ? "runtime" : "editor";
		return FirstExistingOrPreferred({
		    GetProjectTargetArtifactDirectory(request.RepositoryRoot, request.ProjectId, productRole, profileName) / fileName,
		    ResolveSparkleToolPath(request.RepositoryRoot, profileName, targetName),
		});
	}

	std::string ToString(LaunchOperationKind kind)
	{
		switch (kind)
		{
		case LaunchOperationKind::RunProject:
			return "RunProject";
		}

		return "Unknown";
	}

	const std::vector<LaunchOperationDefinition>& GetLaunchOperationDefinitions()
	{
		static const std::vector<LaunchOperationDefinition> definitions = {
		    {LaunchOperationKind::RunProject, "project.open.editor", "Start", "Open Editor", "Launch the selected project in editor mode using available runtime components."},
		    {LaunchOperationKind::RunProject, "project.open.runtime", "Start", "Open Runtime", "Launch the selected project in runtime mode using available runtime components."},
		    {LaunchOperationKind::RunProject, "project.run.smoke", "Run", "Run Smoke Tests", "Run the selected project with smoke validation enabled."},
		    {LaunchOperationKind::RunProject, "project.run", "Run", "Run Project", "Run the selected project in editor or runtime mode, optionally with smoke validation."},
		};
		return definitions;
	}

	std::optional<LaunchOperationDefinition> FindLaunchOperationDefinition(std::string_view operationId)
	{
		const std::vector<LaunchOperationDefinition>& definitions = GetLaunchOperationDefinitions();
		const auto found = std::find_if(definitions.begin(), definitions.end(), [operationId](const LaunchOperationDefinition& definition) {
			return definition.Id == operationId;
		});
		return found == definitions.end() ? std::nullopt : std::optional<LaunchOperationDefinition>(*found);
	}

	LaunchOperationPlan PlanLaunchOperation(std::string_view operationId, const LaunchOperationRequest& request)
	{
		LaunchOperationPlan plan;
		const std::optional<LaunchOperationDefinition> definition = FindLaunchOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown launch operation");
			plan.Operation.FailureSummary = "Unknown launch operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Profile = ResolveLaunchProfile(plan.Kind, request);
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"project", request.ProjectId});
		plan.Operation.Inputs.push_back({"target", IsRuntimeLaunchTarget(request) ? "runtime" : "editor"});
		plan.Operation.Inputs.push_back({"profile", plan.Profile});
		if (request.EnableSmokeTest)
		{
			plan.Operation.Inputs.push_back({"smokeTest", "enabled"});
		}
		if (!request.GraphicsBackend.empty())
		{
			plan.Operation.Inputs.push_back({"graphicsBackend", request.GraphicsBackend});
		}
		if (!request.VSync.empty())
		{
			plan.Operation.Inputs.push_back({"r.VSync", request.VSync});
		}
		if (!request.PreferHighPerformanceAdapter.empty())
		{
			plan.Operation.Inputs.push_back({"r.PreferHighPerformanceAdapter", request.PreferHighPerformanceAdapter});
		}
		if (!request.MeshAutoBatching.empty())
		{
			plan.Operation.Inputs.push_back({"r.MeshAutoBatching", request.MeshAutoBatching});
		}
		if (!request.CustomArguments.empty())
		{
			plan.Operation.Inputs.push_back({"customArguments", std::to_string(request.CustomArguments.size())});
		}
		if (!request.CustomCVars.empty())
		{
			plan.Operation.Inputs.push_back({"customCVars", std::to_string(request.CustomCVars.size())});
		}
		PopulateRhiSmokeLaunchInputs(plan);
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");

		const std::optional<BuildProfile> profile = ResolveProfileForLaunch(request, plan.Profile);
		if (!profile.has_value())
		{
			AddReadiness(plan, "Launch profile does not match the requested launch target: " + plan.Profile);
			return plan;
		}

		plan.TargetName = BuildProjectTargetName(request.ProjectId, *profile);
		plan.ExecutablePath = ResolveLaunchExecutablePath(request, plan.Profile, plan.TargetName);
		plan.WorkingDirectory = request.RepositoryRoot / "Projects" / request.ProjectId;
		PopulateRhiSmokeLaunchEnvironment(plan);

		std::error_code errorCode;
		const bool executableExists = std::filesystem::exists(plan.ExecutablePath, errorCode);
		errorCode.clear();
		const bool projectMarkerExists = std::filesystem::exists(plan.WorkingDirectory / ".sparkle-project", errorCode);
		errorCode.clear();
		const std::filesystem::path cookedProjectDirectory = GetCookedProjectDirectory(request.RepositoryRoot, request.ProjectId);
		const bool cookedMeshesReady = CookedAssetScopeHasFiles(cookedProjectDirectory, "Meshes");
		const bool cookedTexturesReady = CookedAssetScopeHasFiles(cookedProjectDirectory, "Textures");
		const bool cookedShadersReady = CookedAssetScopeHasFiles(cookedProjectDirectory, "Shaders");
		AddReadiness(plan, executableExists ? "Executable is ready." : "Executable is missing; compile the target first: " + plan.TargetName);
		AddReadiness(plan, projectMarkerExists ? "Project working directory is valid." : "Project working directory is missing or is not a Sparkle project: " + plan.WorkingDirectory.string());
		AddReadiness(plan, cookedMeshesReady ? "Cooked scene assets are ready." : "Cooked scene assets are missing; run Cook Scene Assets before launching.");
		AddReadiness(plan, cookedTexturesReady ? "Cooked textures are ready." : "Cooked textures are missing; cook textures before launching.");
		AddReadiness(plan, cookedShadersReady ? "Cooked shaders are ready." : "Cooked shaders are missing; cook shaders before launching.");
		AddPlannedEffect(plan, std::string("Launch ") + (IsRuntimeLaunchTarget(request) ? "runtime" : "editor") + " executable " + plan.ExecutablePath.string() + " with working directory " + plan.WorkingDirectory.string() + ".");
		if (!request.GraphicsBackend.empty())
		{
			AddPlannedEffect(plan, "Use graphics backend: " + request.GraphicsBackend + ".");
		}
		if (!request.VSync.empty())
		{
			AddPlannedEffect(plan, "Set r.VSync=" + request.VSync + ".");
		}
		if (!request.PreferHighPerformanceAdapter.empty())
		{
			AddPlannedEffect(plan, "Set r.PreferHighPerformanceAdapter=" + request.PreferHighPerformanceAdapter + ".");
		}
		if (!request.MeshAutoBatching.empty())
		{
			AddPlannedEffect(plan, "Set r.MeshAutoBatching=" + request.MeshAutoBatching + ".");
		}
		if (!request.CustomArguments.empty())
		{
			AddPlannedEffect(plan, "Append " + std::to_string(request.CustomArguments.size()) + " custom command-line argument(s).");
		}
		for (const std::string& customCVar : request.CustomCVars)
		{
			AddPlannedEffect(plan, "Set " + customCVar + ".");
		}
		for (const std::string& effect : GetRhiSmokeLaunchPlannedEffects(plan))
		{
			AddPlannedEffect(plan, effect);
		}
		plan.CanRun = executableExists && projectMarkerExists && cookedMeshesReady && cookedTexturesReady && cookedShadersReady;
		PopulateLaunchStep(plan);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const LaunchOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			dryRun << "\n    Working directory: " << plan.WorkingDirectory.string();
			for (const EnvironmentOverride& overrideValue : plan.Environment)
			{
				dryRun << "\n    Env: " << overrideValue.Name << "=" << overrideValue.Value;
			}
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
