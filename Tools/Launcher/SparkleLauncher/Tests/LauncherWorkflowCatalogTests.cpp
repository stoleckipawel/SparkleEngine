#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LevelRunOperations.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace SparkleLauncher::LauncherWorkflowCatalogTests
{
	static bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
		}
		return condition;
	}

	static bool LevelsDomainAndHomeProjectionAgree()
	{
		bool passed = true;
		const std::optional<LevelOperationDefinition> levelDefinition = FindLevelOperationDefinition("levels.sync");
		passed &= Expect(levelDefinition.has_value(), "The Levels backend must register levels.sync.");
		passed &= Expect(
		    levelDefinition.has_value() && levelDefinition->Group == "Levels",
		    "The Levels backend operation must use the Levels domain group.");
		passed &= Expect(
		    !FindBuildWorkspaceOperationDefinition("levels.sync").has_value(),
		    "The Levels operation must not be owned by BuildWorkspace.");
		passed &= Expect(
		    !FindBuildWorkspaceOperationDefinition("workspace.sync-levels").has_value(),
		    "The replaced workspace.sync-levels identity must not remain registered.");
		const std::optional<LevelRunOperationDefinition> runDefinition = FindLevelRunOperationDefinition("levels.run");
		passed &= Expect(runDefinition.has_value(), "The Levels backend must register levels.run.");
		passed &= Expect(
		    runDefinition.has_value() && runDefinition->Group == "Levels",
		    "Run Level must remain a backend Levels operation regardless of its frontend placement.");
		LevelRunOperationRequest runRequest;
		runRequest.LevelId = "Sponza";
		runRequest.GraphicsApi = "vulkan";
		const LevelRunOperationPlan runPlan = PlanLevelRunOperation("levels.run", runRequest);
		const bool selectsRequestedLevel = std::any_of(
		    runPlan.Environment.begin(),
		    runPlan.Environment.end(),
		    [](const Process::EnvironmentOverride& environment)
		    { return environment.Name == "SPARKLE_STARTUP_LEVEL" && environment.Value == "Sponza"; });
		passed &=
		    Expect(selectsRequestedLevel, "The final Levels operation must pass the requested catalog identity to the runtime process.");
		const bool selectsRequestedGraphicsApi = std::any_of(
		    runPlan.Operation.Inputs.begin(),
		    runPlan.Operation.Inputs.end(),
		    [](const OperationInput& input) { return input.Name == "graphicsApi" && input.Value == "vulkan"; });
		passed &= Expect(
		    selectsRequestedGraphicsApi,
		    "The final Levels operation must retain the footer-selected graphics API as a typed operation input.");

		const LauncherOperationUiModel uiModel = LauncherUiModelForOperation("levels.sync");
		passed &=
		    Expect(uiModel.PageKind == LauncherWorkflowPageKind::Home, "The Levels operation UI projection must belong to Quick Start.");
		const LauncherOperationUiModel runUiModel = LauncherUiModelForOperation("levels.run");
		passed &= Expect(
		    runUiModel.PageKind == LauncherWorkflowPageKind::Home,
		    "Run Level must project into Quick Start without becoming a separate rail workflow.");

		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		const auto home = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Home; });
		passed &= Expect(home != workflows.end(), "The frontend workflow catalog must contain Quick Start.");
		if (home != workflows.end())
		{
			passed &= Expect(
			    home->OperationIds == QVector<QString>{LauncherHomeOperationId()},
			    "Quick Start must retain one stable page identity.");
		}

		const bool directlyProjectsLevelOperations = std::any_of(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow)
		    { return workflow.OperationIds.contains("levels.sync") || workflow.OperationIds.contains("levels.run"); });
		passed &= Expect(
		    !directlyProjectsLevelOperations,
		    "Level operations must be exposed by Quick Start cards rather than a separate workflow-rail operation.");

		const auto cook = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Cook; });
		passed &= Expect(
		    cook != workflows.end() && cook + 1 != workflows.end() && (cook + 1)->PageKind == LauncherWorkflowPageKind::Clean,
		    "Clean must follow Cook after the separate Levels rail group is removed.");
		return passed;
	}
}

int main()
{
	using namespace SparkleLauncher::LauncherWorkflowCatalogTests;
	if (!LevelsDomainAndHomeProjectionAgree())
	{
		return 1;
	}

	std::cout << "Launcher workflow domain/projection agreement passed.\n";
	return 0;
}
