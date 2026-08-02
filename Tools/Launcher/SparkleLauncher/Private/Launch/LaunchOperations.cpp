#include "SparkleLauncher/LaunchOperations.h"

#include "LaunchOperationProcessRequests.h"
#include "Core/Public/FileSystemUtils.h"
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

	static void AddEnvironment(LaunchOperationPlan& plan, std::string name, std::string value)
	{
		Process::EnvironmentOverride overrideValue;
		overrideValue.Name = std::move(name);
		overrideValue.Value = std::move(value);
		plan.Environment.push_back(std::move(overrideValue));
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
	    const std::filesystem::path& repositoryRoot,
	    std::string_view projectId,
	    std::string_view relativeDirectory)
	{
		const std::string relativeScope(relativeDirectory);
		const std::filesystem::path cookedProjectDirectory = GetCookedProjectDirectory(repositoryRoot, projectId);
		if (DirectoryHasRegularFiles(cookedProjectDirectory / relativeScope))
		{
			return true;
		}

		if (DirectoryHasRegularFiles(GetSharedCookedProjectDirectory(repositoryRoot) / relativeScope))
		{
			return true;
		}

		return false;
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
		    GetProjectTargetArtifactDirectory(request.RepositoryRoot, request.ContentId, productRole, profileName) / fileName,
		    ResolveSparkleToolPath(request.RepositoryRoot, profileName, targetName),
		});
	}

	std::string ToString(LaunchOperationKind kind)
	{
		switch (kind)
		{
			case LaunchOperationKind::RunContent:
				return "RunContent";
		}

		return "Unknown";
	}

	const std::vector<LaunchOperationDefinition>& GetLaunchOperationDefinitions()
	{
		static const std::vector<LaunchOperationDefinition> definitions = {
		    {LaunchOperationKind::RunContent,
		        "launch.editor",
		        "Launch",
		        "Open Editor",
		        "Launch the editor using available runtime components."},
		    {LaunchOperationKind::RunContent,
		        "launch.runtime",
		        "Launch",
		        "Open Runtime",
		        "Launch the runtime using available runtime components."},
		    {LaunchOperationKind::RunContent,
		        "launch.run",
		        "Launch",
		        "Launch",
		        "Launch the editor or runtime using shared launch options."},
		};
		return definitions;
	}

	std::optional<LaunchOperationDefinition> FindLaunchOperationDefinition(std::string_view operationId)
	{
		const std::vector<LaunchOperationDefinition>& definitions = GetLaunchOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const LaunchOperationDefinition& definition) { return definition.Id == operationId; });
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
		plan.Profile = ResolveLaunchProfile(plan.Kind, plan.Request);
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"content", plan.Request.ContentId});
		plan.Operation.Inputs.push_back({"target", IsRuntimeLaunchTarget(plan.Request) ? "runtime" : "editor"});
		plan.Operation.Inputs.push_back({"profile", plan.Profile});
		if (!plan.Request.StartupLevel.empty())
		{
			plan.Operation.Inputs.push_back({"startupLevel", plan.Request.StartupLevel});
		}
		if (!plan.Request.GraphicsBackend.empty())
		{
			plan.Operation.Inputs.push_back({"graphicsBackend", plan.Request.GraphicsBackend});
		}
		if (!plan.Request.VSync.empty())
		{
			plan.Operation.Inputs.push_back({"r.VSync", plan.Request.VSync});
		}
		if (!plan.Request.PreferHighPerformanceAdapter.empty())
		{
			plan.Operation.Inputs.push_back({"r.PreferHighPerformanceAdapter", plan.Request.PreferHighPerformanceAdapter});
		}
		if (!plan.Request.CustomArguments.empty())
		{
			plan.Operation.Inputs.push_back({"customArguments", std::to_string(plan.Request.CustomArguments.size())});
		}
		if (!plan.Request.CustomCVars.empty())
		{
			plan.Operation.Inputs.push_back({"customCVars", std::to_string(plan.Request.CustomCVars.size())});
		}
		plan.Operation.LogPath = GetLauncherOperationLogPath(plan.Request.RepositoryRoot, definition->Id, "Latest.txt");

		const std::optional<BuildProfile> profile = ResolveProfileForLaunch(plan.Request, plan.Profile);
		if (!profile.has_value())
		{
			AddReadiness(plan, "Launch profile does not match the requested launch target: " + plan.Profile);
			return plan;
		}

		plan.TargetName = BuildProjectTargetName(plan.Request.ContentId, *profile);
		plan.ExecutablePath = ResolveLaunchExecutablePath(plan.Request, plan.Profile, plan.TargetName);
		plan.WorkingDirectory = plan.Request.RepositoryRoot / "Projects" / plan.Request.ContentId;
		if (!plan.Request.StartupLevel.empty())
		{
			AddEnvironment(plan, "SPARKLE_STARTUP_LEVEL", plan.Request.StartupLevel);
		}

		std::error_code errorCode;
		const bool executableExists = std::filesystem::exists(plan.ExecutablePath, errorCode);
		errorCode.clear();
		const bool projectMarkerExists =
		    std::filesystem::exists(plan.WorkingDirectory / std::string(Filesystem::kProjectMarker), errorCode);
		errorCode.clear();
		const bool cookedMeshesReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Meshes");
		const bool cookedTexturesReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Textures");
		const bool cookedShadersReady = CookedAssetScopeHasFiles(plan.Request.RepositoryRoot, plan.Request.ContentId, "Shaders");
		plan.Readiness.ExecutableReady = executableExists;
		plan.Readiness.ContentDirectoryReady = projectMarkerExists;
		plan.Readiness.CookedMeshesReady = cookedMeshesReady;
		plan.Readiness.CookedTexturesReady = cookedTexturesReady;
		plan.Readiness.CookedShadersReady = cookedShadersReady;
		AddReadiness(
		    plan,
		    executableExists ? "Executable is ready." : "Executable is missing; compile the target first: " + plan.TargetName);
		AddReadiness(
		    plan,
		    projectMarkerExists ? "Content working directory is valid."
		                        : "Content working directory is missing or invalid: " + plan.WorkingDirectory.string());
		AddReadiness(
		    plan,
		    cookedMeshesReady ? "Cooked scenes and meshes are ready."
		                      : "Cooked scenes and meshes are missing; run Cook Scenes And Meshes before launching.");
		AddReadiness(
		    plan,
		    cookedTexturesReady ? "Cooked textures are ready." : "Cooked textures are missing; run Cook Textures before launching.");
		AddReadiness(
		    plan,
		    cookedShadersReady ? "Cooked shaders are ready." : "Cooked shaders are missing; run Cook Shaders before launching.");
		AddPlannedEffect(
		    plan,
		    std::string("Launch ") + (IsRuntimeLaunchTarget(plan.Request) ? "runtime" : "editor") + " executable "
		        + plan.ExecutablePath.string() + " with working directory " + plan.WorkingDirectory.string() + ".");
		if (!plan.Request.GraphicsBackend.empty())
		{
			AddPlannedEffect(plan, "Use graphics backend: " + plan.Request.GraphicsBackend + ".");
		}
		if (!plan.Request.VSync.empty())
		{
			AddPlannedEffect(plan, "Set r.VSync=" + plan.Request.VSync + ".");
		}
		if (!plan.Request.PreferHighPerformanceAdapter.empty())
		{
			AddPlannedEffect(plan, "Set r.PreferHighPerformanceAdapter=" + plan.Request.PreferHighPerformanceAdapter + ".");
		}
		if (!plan.Request.StartupLevel.empty())
		{
			AddPlannedEffect(plan, "Use startup level: " + plan.Request.StartupLevel + ".");
		}
		if (!plan.Request.CustomArguments.empty())
		{
			AddPlannedEffect(plan, "Append " + std::to_string(plan.Request.CustomArguments.size()) + " custom command-line argument(s).");
		}
		for (const std::string& customCVar : plan.Request.CustomCVars)
		{
			AddPlannedEffect(plan, "Set " + customCVar + ".");
		}
		plan.CanRun = executableExists && projectMarkerExists && cookedMeshesReady && cookedTexturesReady && cookedShadersReady;
		PopulateLaunchStep(plan);

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const LaunchOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			dryRun << "\n    Working directory: " << plan.WorkingDirectory.string();
			for (const Process::EnvironmentOverride& overrideValue : plan.Environment)
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
