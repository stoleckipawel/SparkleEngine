#pragma once

#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"
#include "SparkleLauncher/LauncherContentDefaults.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class ToolchainItemState : std::uint8_t
	{
		Found,
		Missing,
		Warning
	};

	enum class WorkspaceCompiler : std::uint8_t
	{
		Msvc,
		ClangCl
	};

	struct ToolchainItemStatus
	{
		std::string Id;
		std::string DisplayName;
		bool Required = true;
		ToolchainItemState State = ToolchainItemState::Missing;
		std::filesystem::path Path;
		std::string Detail;
		std::optional<WorkspaceCompiler> Compiler;
		bool CanInstall = false;
	};

	struct BuildToolchainStatus
	{
		std::string Generator;
		std::string Platform = "x64";
		std::string Toolset;
		std::filesystem::path CMakePath;
		std::filesystem::path MSBuildPath;
		std::filesystem::path NinjaPath;
		std::filesystem::path RiderPath;
		std::filesystem::path GitPath;
		std::filesystem::path VswherePath;
		std::filesystem::path VisualStudioPath;
		std::filesystem::path VisualStudioIdePath;
		std::filesystem::path VisualStudioInstallerPath;
		std::filesystem::path ClangClPath;
		std::filesystem::path QtRootPath;
		std::filesystem::path QtQmakePath;
		std::filesystem::path ShaderCompilerSdkRoot;
		std::filesystem::path VulkanSdkRoot;
		std::string WindowsSdkVersion;
		std::vector<ToolchainItemStatus> Items;
		WorkspaceCompiler Compiler = WorkspaceCompiler::Msvc;
		bool RequiredToolsAvailable = false;
		bool ConfigurePrerequisitesAvailable = true;
	};

	struct WorkspaceFeatureSettings
	{
		bool ContentPipelineEnabled = false;
		bool ShaderCompilerEnabled = false;
		bool KtxSupportEnabled = false;
		bool NvidiaStreamlineEnabled = false;
	};

	enum class WorkspaceIde : std::uint8_t
	{
		VisualStudio,
		Rider
	};

	enum class BuildFilesFreshnessState : std::uint8_t
	{
		Current,
		BuildDirectoryMissing,
		CMakeCacheMissing,
		SolutionMissing,
		GeneratorMismatch,
		FeatureSetMismatch,
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

	enum class BuildWorkspaceOperationKind : std::uint8_t
	{
		SyncCode,
		GenerateBuildFiles,
		BuildWorkspace,
		CompileLauncher,
		CompileEditor,
		CompileRuntime,
		BuildCookTools,
		InstallHostTool
	};

	enum class BuildWorkspaceScope : std::uint8_t
	{
		Editor,
		Runtime,
		CookTools,
		Launcher
	};

	struct BuildWorkspaceOperationDefinition
	{
		BuildWorkspaceOperationKind Kind = BuildWorkspaceOperationKind::SyncCode;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct BuildWorkspaceOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ContentId = kDefaultContentId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		WorkspaceIde PreferredIde = WorkspaceIde::VisualStudio;
		WorkspaceCompiler Compiler = WorkspaceCompiler::Msvc;
		std::vector<BuildWorkspaceScope> SelectedScopes = {
		    BuildWorkspaceScope::Editor,
		    BuildWorkspaceScope::Runtime,
		    BuildWorkspaceScope::CookTools};
		std::vector<std::string> SelectedTargets;
		std::string SourceDependencyId;
		std::string HostToolId;
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
		BuildWorkspaceOperationKind Kind = BuildWorkspaceOperationKind::SyncCode;
		BuildToolchainStatus Toolchain;
		BuildFilesFreshnessStatus Freshness;
		SourceDependencyInventoryStatus SourceDependencies;
		std::vector<BuildWorkspaceOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(ToolchainItemState state);
	std::string ToString(BuildFilesFreshnessState state);
	std::string ToString(BuildWorkspaceOperationKind kind);
	std::string BuildWorkspaceScopeId(BuildWorkspaceScope scope);
	bool TryParseBuildWorkspaceScope(std::string_view text, BuildWorkspaceScope& outScope);
	std::string ToString(WorkspaceIde ide);
	std::string DisplayName(WorkspaceIde ide);
	std::string WorkspaceIdeCommandLineValue(WorkspaceIde ide);
	bool TryParseWorkspaceIde(std::string_view text, WorkspaceIde& outIde);
	std::string ToString(WorkspaceCompiler compiler);
	std::string DisplayName(WorkspaceCompiler compiler);
	std::string WorkspaceCompilerCommandLineValue(WorkspaceCompiler compiler);
	bool TryParseWorkspaceCompiler(std::string_view text, WorkspaceCompiler& outCompiler);
	WorkspaceFeatureSettings GetLauncherWorkspaceFeatureSettings();
	bool HasIncompleteEnabledSourceDependencies(const BuildWorkspaceOperationPlan& plan);
	bool BuildWorkspaceOperationRequiresConfigureStep(const BuildWorkspaceOperationPlan& plan);
	const std::vector<BuildWorkspaceOperationDefinition>& GetBuildWorkspaceOperationDefinitions();
	std::optional<BuildWorkspaceOperationDefinition> FindBuildWorkspaceOperationDefinition(std::string_view operationId);
	BuildToolchainStatus DetectBuildToolchain(
	    const std::filesystem::path& repositoryRoot,
	    WorkspaceIde preferredIde,
	    WorkspaceCompiler compiler = WorkspaceCompiler::Msvc);
	BuildFilesFreshnessStatus CheckBuildFilesFreshness(const std::filesystem::path& repositoryRoot, const BuildToolchainStatus& toolchain);
	bool UpdateBuildFilesFreshnessStamp(
	    const std::filesystem::path& repositoryRoot,
	    const BuildToolchainStatus& toolchain,
	    std::string& errorMessage);
	BuildWorkspaceOperationPlan PlanBuildWorkspaceOperation(std::string_view operationId, const BuildWorkspaceOperationRequest& request);
	OperationRecord RunBuildWorkspaceOperationPlan(
	    BuildWorkspaceOperationPlan plan,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback = {});
}
