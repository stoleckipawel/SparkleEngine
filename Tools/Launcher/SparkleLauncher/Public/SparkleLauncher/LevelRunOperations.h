#pragma once

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherContentDefaults.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	enum class LevelRunMode : std::uint8_t
	{
		Editor,
		Game
	};

	struct LevelRunOperationDefinition
	{
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct LevelRunOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ContentId = kDefaultContentId;
		LevelRunMode RunMode = LevelRunMode::Editor;
		std::string ProductProfile = "DevelopmentEditor";
		std::string LevelId;
		std::string GraphicsApi = "d3d12";
	};

	struct LevelRunOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
	};

	struct LevelRunReadinessState
	{
		bool ExecutableReady = false;
		bool ContentDirectoryReady = false;
		bool CookedMeshesReady = false;
		bool CookedTexturesReady = false;
		bool CookedShadersReady = false;
	};

	struct LevelRunOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		LevelRunOperationRequest Request;
		std::string TargetName;
		std::filesystem::path ExecutablePath;
		std::filesystem::path WorkingDirectory;
		std::vector<Process::EnvironmentOverride> Environment;
		std::vector<LevelRunOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		LevelRunReadinessState Readiness;
		bool CanRun = false;
	};

	std::string_view ToString(LevelRunMode mode) noexcept;
	const std::vector<LevelRunOperationDefinition>& GetLevelRunOperationDefinitions();
	std::optional<LevelRunOperationDefinition> FindLevelRunOperationDefinition(std::string_view operationId);
	LevelRunOperationPlan PlanLevelRunOperation(std::string_view operationId, const LevelRunOperationRequest& request);
	OperationRecord RunLevelRunOperationPlan(
	    LevelRunOperationPlan plan,
	    IProcessRunner& processRunner,
	    const ProcessOutputCallback& outputCallback = {});
}
