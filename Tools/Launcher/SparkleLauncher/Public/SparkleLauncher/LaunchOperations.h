#pragma once

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherProjectDefaults.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class LaunchOperationKind
	{
		RunProject
	};

	struct LaunchOperationDefinition
	{
		LaunchOperationKind Kind = LaunchOperationKind::RunProject;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct LaunchOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string OperationId = "project.run";
		std::string ProjectId = kDefaultProjectId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::string Target = "editor";
		std::string StartupLevel = "Sponza";
		bool EnableSmokeTest = false;
		std::string GraphicsBackend;
		std::string VSync;
		std::string PreferHighPerformanceAdapter;
		std::string MeshAutoBatching;
		std::string PreferPartitionedTlas = "true";
		std::string PtlasOperationWriterPath = "1";
		std::vector<std::string> CustomArguments;
		std::vector<std::string> CustomCVars;
		std::string SmokeBackend;
		std::string SmokeFrameLimit;
		std::string SmokeViewMode;
		std::string SmokeCapturePath;
		bool SmokeTrace = false;
		bool SmokeSkipLevelSwitching = false;
		bool SmokeRunRayTracingParity = false;
		bool SmokeRunPtlasBenchmark = false;
		bool SmokeRunDiagnosticCaptures = false;
	};

	struct LaunchOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
	};

	struct LaunchOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		LaunchOperationRequest Request;
		LaunchOperationKind Kind = LaunchOperationKind::RunProject;
		std::string Profile;
		std::string TargetName;
		std::filesystem::path ExecutablePath;
		std::filesystem::path WorkingDirectory;
		std::vector<EnvironmentOverride> Environment;
		std::vector<LaunchOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(LaunchOperationKind kind);
	const std::vector<LaunchOperationDefinition>& GetLaunchOperationDefinitions();
	std::optional<LaunchOperationDefinition> FindLaunchOperationDefinition(std::string_view operationId);
	LaunchOperationPlan PlanLaunchOperation(std::string_view operationId, const LaunchOperationRequest& request);
	OperationRecord RunLaunchOperationPlan(LaunchOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback = {});
}
