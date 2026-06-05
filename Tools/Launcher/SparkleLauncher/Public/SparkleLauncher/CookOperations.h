#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
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
	enum class CookOperationKind
	{
		CookShaders,
		BuildTextures,
		BuildSceneAssets,
		CookAllAssets
	};

	enum class CookMode
	{
		Incremental,
		Force
	};

	struct CookOperationDefinition
	{
		CookOperationKind Kind = CookOperationKind::CookAllAssets;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct CookOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ProjectId = kDefaultProjectId;
		std::string RuntimeProfile = "DevelopmentGame";
		CookMode Mode = CookMode::Incremental;
		bool ForceRecookConfirmed = false;
		std::vector<std::string> ShaderPackages;
		std::vector<std::string> ShaderTargets;
		std::string ShaderBackend = "auto";
		std::filesystem::path ShaderCacheDirectory;
		std::filesystem::path ShaderDebugArtifactDirectory;
		bool ShaderUseCache = true;
		bool ShaderEnableDebugInfo = false;
		bool ShaderEnableOptimizations = true;
		bool ShaderWarningsAsErrors = true;
		bool ShaderStripReflection = true;
		bool ShaderStripDebugInfo = true;
		bool WriteCookedShaderStats = false;
	};

	struct CookOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
		std::filesystem::path DestructivePath;
		bool Destructive = false;
	};

	struct CookOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		CookOperationRequest Request;
		CookOperationKind Kind = CookOperationKind::CookAllAssets;
		BuildToolchainStatus Toolchain;
		BuildFilesFreshnessStatus Freshness;
		std::string ToolProfile;
		std::vector<std::filesystem::path> RequiredToolPaths;
		std::filesystem::path CookedOutputDirectory;
		std::vector<CookOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(CookOperationKind kind);
	std::string ToString(CookMode mode);
	const std::vector<CookOperationDefinition>& GetCookOperationDefinitions();
	std::optional<CookOperationDefinition> FindCookOperationDefinition(std::string_view operationId);
	CookOperationPlan PlanCookOperation(std::string_view operationId, const CookOperationRequest& request);
	OperationRecord RunCookOperationPlan(CookOperationPlan plan, IProcessRunner& processRunner, ProcessOutputCallback outputCallback = {});
}
