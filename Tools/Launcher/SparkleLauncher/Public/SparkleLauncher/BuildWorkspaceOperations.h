#pragma once

#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class ToolchainItemState
	{
		Found,
		Missing,
		Warning
	};

	struct ToolchainItemStatus
	{
		std::string Id;
		std::string DisplayName;
		bool Required = true;
		ToolchainItemState State = ToolchainItemState::Missing;
		std::filesystem::path Path;
		std::string Detail;
	};

	struct BuildToolchainStatus
	{
		std::string Generator;
		std::string Platform = "x64";
		std::string Toolset;
		std::filesystem::path CMakePath;
		std::filesystem::path MSBuildPath;
		std::filesystem::path GitPath;
		std::filesystem::path ClangFormatPath;
		std::filesystem::path VswherePath;
		std::string WindowsSdkVersion;
		std::vector<ToolchainItemStatus> Items;
		bool RequiredToolsAvailable = false;
	};

	enum class BuildFilesFreshnessState
	{
		Current,
		BuildDirectoryMissing,
		CMakeCacheMissing,
		SolutionMissing,
		GeneratorMismatch,
		FreshnessStampMissing,
		FreshnessStampMismatch,
		SourceListChanged,
		BuildInputChanged,
		Unsupported
	};

	struct BuildFilesFreshnessStatus
	{
		BuildFilesFreshnessState State = BuildFilesFreshnessState::Unsupported;
		bool Current = false;
		std::filesystem::path BuildDirectory;
		std::filesystem::path CachePath;
		std::filesystem::path SolutionPath;
		std::filesystem::path StampPath;
		std::string Summary;
	};

	enum class BuildWorkspaceOperationKind
	{
		CheckToolchain,
		SetupWorkspace,
		GenerateSolution,
		OpenSolution,
		CompileEditor,
		CompileRuntime,
		BuildCookTools
	};

	struct BuildWorkspaceOperationDefinition
	{
		BuildWorkspaceOperationKind Kind = BuildWorkspaceOperationKind::CheckToolchain;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct BuildWorkspaceOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ProjectId = "Showcase";
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		std::vector<std::string> SelectedTargets;
		bool ForceConfigure = false;
	};

	struct BuildWorkspaceOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
		bool UpdatesBuildFilesFreshness = false;
	};

	struct BuildWorkspaceOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		BuildWorkspaceOperationRequest Request;
		BuildWorkspaceOperationKind Kind = BuildWorkspaceOperationKind::CheckToolchain;
		BuildToolchainStatus Toolchain;
		BuildFilesFreshnessStatus Freshness;
		std::vector<BuildWorkspaceOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(ToolchainItemState state);
	std::string ToString(BuildFilesFreshnessState state);
	std::string ToString(BuildWorkspaceOperationKind kind);
	const std::vector<BuildWorkspaceOperationDefinition>& GetBuildWorkspaceOperationDefinitions();
	std::optional<BuildWorkspaceOperationDefinition> FindBuildWorkspaceOperationDefinition(std::string_view operationId);
	BuildToolchainStatus DetectBuildToolchain(const std::filesystem::path& repositoryRoot);
	BuildFilesFreshnessStatus CheckBuildFilesFreshness(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain);
	bool UpdateBuildFilesFreshnessStamp(const std::filesystem::path& repositoryRoot, const BuildToolchainStatus& toolchain, std::string& errorMessage);
	BuildWorkspaceOperationPlan PlanBuildWorkspaceOperation(
	    std::string_view operationId,
	    const BuildWorkspaceOperationRequest& request);
	OperationRecord RunBuildWorkspaceOperationPlan(
	    BuildWorkspaceOperationPlan plan,
	    IProcessRunner& processRunner,
	    ProcessOutputCallback outputCallback = {});
}