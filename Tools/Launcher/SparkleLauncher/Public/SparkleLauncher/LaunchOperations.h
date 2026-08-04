#pragma once

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherContentDefaults.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class LaunchProduct
	{
		Editor,
		Runtime
	};

	struct LaunchOperationDefinition
	{
		LaunchProduct Product = LaunchProduct::Editor;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct LaunchOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ContentId = kDefaultContentId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::string StartupLevel;
		std::string GraphicsApi = "d3d12";
	};

	struct LaunchOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
	};

	struct LaunchReadinessState
	{
		bool ExecutableReady = false;
		bool ContentDirectoryReady = false;
		bool CookedMeshesReady = false;
		bool CookedTexturesReady = false;
		bool CookedShadersReady = false;
	};

	struct LaunchOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		LaunchOperationRequest Request;
		LaunchProduct Product = LaunchProduct::Editor;
		std::string Profile;
		std::string TargetName;
		std::filesystem::path ExecutablePath;
		std::filesystem::path WorkingDirectory;
		std::vector<Process::EnvironmentOverride> Environment;
		std::vector<LaunchOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		LaunchReadinessState Readiness;
		bool CanRun = false;
	};

	const std::vector<LaunchOperationDefinition>& GetLaunchOperationDefinitions();
	std::optional<LaunchOperationDefinition> FindLaunchOperationDefinition(std::string_view operationId);
	LaunchOperationPlan PlanLaunchOperation(std::string_view operationId, const LaunchOperationRequest& request);
	OperationRecord RunLaunchOperationPlan(
	    LaunchOperationPlan plan,
	    IProcessRunner& processRunner,
	    ProcessOutputCallback outputCallback = {});
}
