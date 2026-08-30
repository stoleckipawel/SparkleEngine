#include "SparkleLauncher/LevelOperations.h"

#include "LevelOperationProcessRequests.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Strings/StringUtils.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/ToolResolver.h"

#include <algorithm>
#include <sstream>
#include <utility>

namespace SparkleLauncher
{
	static void AddReadiness(LevelOperationPlan& plan, std::string message)
	{
		plan.ReadinessMessages.push_back(std::move(message));
	}

	static void PopulateLevelOperationSteps(LevelOperationPlan& plan)
	{
		for (const LevelOperationProcessStep& processStep : BuildLevelOperationProcessSteps(plan))
		{
			LevelOperationStep step;
			step.Id = processStep.Id;
			step.DisplayName = processStep.DisplayName;
			step.DisplayCommandLine = BuildDisplayCommandLine(processStep.Request.ExecutablePath, processStep.Request.Arguments);
			step.LogPath = processStep.Request.LogPath;
			plan.Steps.push_back(std::move(step));
		}
	}

	std::string ToString(LevelOperationKind kind)
	{
		switch (kind)
		{
			case LevelOperationKind::Sync:
				return "Sync";
		}

		return "Unknown";
	}

	const std::vector<LevelOperationDefinition>& GetLevelOperationDefinitions()
	{
		static const std::vector<LevelOperationDefinition> definitions = {
		    {LevelOperationKind::Sync,
		        "levels.sync",
		        "Levels",
		        "Sync Levels",
		        "Select levels and acquire their asset packs without changing code, workspace, or SDK dependencies."},
		};
		return definitions;
	}

	std::optional<LevelOperationDefinition> FindLevelOperationDefinition(std::string_view operationId)
	{
		const std::vector<LevelOperationDefinition>& definitions = GetLevelOperationDefinitions();
		const auto found = std::find_if(
		    definitions.begin(),
		    definitions.end(),
		    [operationId](const LevelOperationDefinition& definition) { return definition.Id == operationId; });
		return found == definitions.end() ? std::nullopt : std::optional<LevelOperationDefinition>(*found);
	}

	LevelOperationPlan PlanLevelOperation(std::string_view operationId, const LevelOperationRequest& request)
	{
		LevelOperationPlan plan;
		const std::optional<LevelOperationDefinition> definition = FindLevelOperationDefinition(operationId);
		if (!definition.has_value())
		{
			plan.Operation = MakeOperationRecord(std::string(operationId), "Unknown level operation");
			plan.Operation.FailureSummary = "Unknown level operation id.";
			AddReadiness(plan, plan.Operation.FailureSummary);
			return plan;
		}

		plan.Kind = definition->Kind;
		plan.RepositoryRoot = request.RepositoryRoot;
		plan.Request = request;
		plan.Operation = MakeOperationRecord(definition->Id, definition->DisplayName);
		plan.Operation.Inputs.push_back({"content", request.ContentId});
		if (!request.RequestedLevelIds.empty())
		{
			std::vector<std::string_view> levelIds;
			levelIds.reserve(request.RequestedLevelIds.size());
			for (const std::string& levelId : request.RequestedLevelIds)
			{
				levelIds.push_back(levelId);
			}
			plan.Operation.Inputs.push_back({"levels", Strings::Join(levelIds, ", ")});
		}
		plan.Operation.LogPath = GetLauncherOperationLogPath(request.RepositoryRoot, definition->Id, "Latest.txt");

		const ToolResolveResult cmake = ResolveKnownTool(KnownTool::CMake);
		if (cmake.Found)
		{
			plan.CMakePath = cmake.Path;
			AddReadiness(plan, "CMake is available for level asset acquisition.");
		}
		else
		{
			AddReadiness(plan, cmake.FailureReason);
		}
		if (request.ContentId.empty())
		{
			AddReadiness(plan, "Repository content is unavailable for level synchronization.");
		}

		plan.PlannedEffects.push_back(
		    "Acquire asset packs referenced by selected maps into gitignored content roots; unselected and disabled packs remain "
		    "untouched.");
		if (cmake.Found && !request.ContentId.empty())
		{
			try
			{
				PopulateLevelOperationSteps(plan);
				plan.CanRun = true;
			}
			catch (const Diagnostics::Error& error)
			{
				AddReadiness(plan, error.what());
			}
		}

		std::ostringstream dryRun;
		dryRun << "Dry-run plan for " << definition->DisplayName << ":";
		for (const LevelOperationStep& step : plan.Steps)
		{
			dryRun << "\n  " << step.DisplayName << ": " << step.DisplayCommandLine;
			dryRun << "\n    Log: " << step.LogPath.string();
		}
		if (plan.Steps.empty())
		{
			dryRun
			    << (plan.CanRun ? "\n  All requested level asset packs are already present."
			                    : "\n  No acquisition step available until readiness issues are resolved.");
		}
		plan.Operation.DryRunText = dryRun.str();
		return plan;
	}
}
