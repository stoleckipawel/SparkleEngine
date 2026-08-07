#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/LevelOperations.h"

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

	static bool LevelsDomainAndProjectionAgree()
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

		const LauncherOperationUiModel uiModel = LauncherUiModelForOperation("levels.sync");
		passed &= Expect(
		    uiModel.PageKind == LauncherWorkflowPageKind::Levels,
		    "The Levels operation UI projection must use the Levels page kind.");

		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		const auto levels = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Levels; });
		passed &= Expect(levels != workflows.end(), "The frontend workflow catalog must contain a Levels projection.");
		if (levels != workflows.end())
		{
			passed &= Expect(levels->OperationIds == QVector<QString>{"levels.sync"}, "Levels must project only levels.sync.");
			passed &= Expect(
			    levels != workflows.begin() && (levels - 1)->PageKind == LauncherWorkflowPageKind::Cook,
			    "Levels must remain directly below Cook in the frontend projection.");
		}

		const auto sync = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Sync; });
		passed &= Expect(sync != workflows.end(), "The frontend workflow catalog must contain Sync.");
		passed &= Expect(
		    sync != workflows.end() && !sync->OperationIds.contains("levels.sync"),
		    "Sync must not visually own the Levels operation.");
		return passed;
	}
}

int main()
{
	using namespace SparkleLauncher::LauncherWorkflowCatalogTests;
	if (!LevelsDomainAndProjectionAgree())
	{
		return 1;
	}

	std::cout << "Launcher workflow domain/projection agreement passed.\n";
	return 0;
}
