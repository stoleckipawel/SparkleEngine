#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <cstdint>
#include <vector>

namespace SparkleLauncher
{
	enum class MaintenanceOperationKind
	{
		RunClangFormat,
		CleanWorkspace
	};

	enum class FormatMode
	{
		Check,
		Apply
	};

	enum class CleanScope
	{
		SelectedProjectCookedOutputs,
		AllCookedOutputs,
		BuildTree,
		ShaderCache,
		ThirdPartyDependencyCache,
		Logs,
		PristineGeneratedWorkspace
	};

	struct MaintenanceOperationDefinition
	{
		MaintenanceOperationKind Kind = MaintenanceOperationKind::RunClangFormat;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct MaintenanceCleanTarget
	{
		std::string DisplayName;
		std::filesystem::path Path;
		std::string Detail;
		std::uintmax_t FileCount = 0;
		std::uintmax_t DirectoryCount = 0;
		std::uintmax_t ByteCount = 0;
		bool Exists = false;
	};

	struct MaintenanceOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ProjectId = "Showcase";
		std::string EditorProfile = "DevelopmentEditor";
		FormatMode RequestedFormatMode = FormatMode::Check;
		CleanScope RequestedCleanScope = CleanScope::SelectedProjectCookedOutputs;
		std::vector<CleanScope> RequestedCleanScopes;
		bool DestructiveActionConfirmed = false;
	};

	struct MaintenanceOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
		std::filesystem::path DestructivePath;
		bool Destructive = false;
	};

	struct MaintenanceOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		MaintenanceOperationRequest Request;
		MaintenanceOperationKind Kind = MaintenanceOperationKind::RunClangFormat;
		BuildToolchainStatus Toolchain;
		BuildFilesFreshnessStatus Freshness;
		std::vector<std::filesystem::path> FormatSourceFiles;
		std::vector<MaintenanceCleanTarget> CleanTargets;
		std::vector<MaintenanceOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(MaintenanceOperationKind kind);
	std::string ToString(FormatMode mode);
	std::string ToString(CleanScope scope);
	const std::vector<MaintenanceOperationDefinition>& GetMaintenanceOperationDefinitions();
	std::optional<MaintenanceOperationDefinition> FindMaintenanceOperationDefinition(std::string_view operationId);
	MaintenanceOperationPlan PlanMaintenanceOperation(std::string_view operationId, const MaintenanceOperationRequest& request);
	OperationRecord RunMaintenanceOperationPlan(MaintenanceOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback = {});
}