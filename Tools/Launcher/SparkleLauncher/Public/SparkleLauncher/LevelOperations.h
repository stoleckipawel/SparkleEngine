#pragma once

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
	enum class LevelOperationKind
	{
		Sync
	};

	struct LevelOperationDefinition
	{
		LevelOperationKind Kind = LevelOperationKind::Sync;
		std::string Id;
		std::string Group;
		std::string DisplayName;
		std::string Description;
	};

	struct LevelOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		std::string ContentId = kDefaultContentId;
		std::vector<std::string> RequestedLevelIds;
	};

	struct LevelOperationStep
	{
		std::string Id;
		std::string DisplayName;
		std::string DisplayCommandLine;
		std::filesystem::path LogPath;
	};

	struct LevelOperationPlan
	{
		OperationRecord Operation;
		std::filesystem::path RepositoryRoot;
		LevelOperationRequest Request;
		LevelOperationKind Kind = LevelOperationKind::Sync;
		std::filesystem::path CMakePath;
		std::vector<LevelOperationStep> Steps;
		std::vector<std::string> PlannedEffects;
		std::vector<std::string> ReadinessMessages;
		bool CanRun = false;
	};

	std::string ToString(LevelOperationKind kind);
	const std::vector<LevelOperationDefinition>& GetLevelOperationDefinitions();
	std::optional<LevelOperationDefinition> FindLevelOperationDefinition(std::string_view operationId);
	LevelOperationPlan PlanLevelOperation(std::string_view operationId, const LevelOperationRequest& request);
	OperationRecord RunLevelOperationPlan(
	    LevelOperationPlan plan,
	    IProcessRunner& processRunner,
	    ProcessOutputCallback outputCallback = {});
}
