#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LevelOperations.h"
#include "SparkleLauncher/LevelRunOperations.h"

#include <algorithm>
#include <filesystem>
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
		runRequest.RepositoryRoot = "C:/Sparkle";
		runRequest.ContentId = "Showcase";
		runRequest.LevelId = "Sponza";
		runRequest.GraphicsApi = "vulkan";
		const LevelRunOperationPlan runPlan = PlanLevelRunOperation("levels.run", runRequest);
		passed &= Expect(runPlan.Request.RunMode == LevelRunMode::Editor, "Editor must be the default level Run Mode.");
		passed &= Expect(runPlan.TargetName == "ShowcaseEditor", "Editor Run Mode must select the editor product target.");
		passed &= Expect(
		    runPlan.ExecutablePath
		        == std::filesystem::path("C:/Sparkle/artifacts/dev/projects/Showcase/editor/DevelopmentEditor/ShowcaseEditor.exe"),
		    "Editor Run Mode must resolve the editor artifact contract.");
		const bool selectsRequestedLevel = std::any_of(
		    runPlan.Environment.begin(),
		    runPlan.Environment.end(),
		    [](const Process::EnvironmentOverride& environment)
		    { return environment.Name == "SPARKLE_STARTUP_LEVEL" && environment.Value == "Sponza"; });
		passed &=
		    Expect(selectsRequestedLevel, "The final Levels operation must pass the requested catalog identity to the selected product.");
		const bool selectsRequestedGraphicsApi = std::any_of(
		    runPlan.Operation.Inputs.begin(),
		    runPlan.Operation.Inputs.end(),
		    [](const OperationInput& input) { return input.Name == "graphicsApi" && input.Value == "vulkan"; });
		passed &= Expect(
		    selectsRequestedGraphicsApi,
		    "The final Levels operation must retain the footer-selected graphics API as a typed operation input.");

		LevelRunOperationRequest gameRequest = runRequest;
		gameRequest.RunMode = LevelRunMode::Game;
		gameRequest.ProductProfile = "DevelopmentGame";
		const LevelRunOperationPlan gamePlan = PlanLevelRunOperation("levels.run", gameRequest);
		passed &= Expect(gamePlan.TargetName == "ShowcaseRuntime", "Game Run Mode must select the standalone product target.");
		passed &= Expect(
		    gamePlan.ExecutablePath
		        == std::filesystem::path("C:/Sparkle/artifacts/dev/projects/Showcase/runtime/DevelopmentGame/ShowcaseRuntime.exe"),
		    "Game Run Mode must resolve the standalone artifact contract.");

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

	static bool BuildUsesOneSelectableWorkspaceIntent()
	{
		bool passed = true;
		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		const auto build = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Build; });
		passed &= Expect(build != workflows.end(), "The frontend workflow catalog must contain Build.");
		if (build != workflows.end())
		{
			passed &= Expect(
			    build->OperationIds == QVector<QString>{"workspace.build"},
			    "Build must project one selectable workspace build intent instead of backend operation tabs.");
		}

		const std::optional<BuildWorkspaceOperationDefinition> definition = FindBuildWorkspaceOperationDefinition("workspace.build");
		passed &= Expect(definition.has_value(), "The BuildWorkspace backend must register workspace.build.");
		passed &= Expect(
		    definition.has_value() && definition->Kind == BuildWorkspaceOperationKind::BuildWorkspace,
		    "workspace.build must use the composite BuildWorkspace planner.");
		passed &= Expect(
		    !FindBuildWorkspaceOperationDefinition("workspace.build-all").has_value(),
		    "The replaced workspace.build-all identity must not remain registered.");
		passed &= Expect(
		    FindBuildWorkspaceOperationDefinition("workspace.build.editor").has_value()
		        && FindBuildWorkspaceOperationDefinition("workspace.build.runtime").has_value()
		        && FindBuildWorkspaceOperationDefinition("cook.tools.prepare").has_value(),
		    "Quick Start build capability primitives must remain registered below the frontend projection.");

		const LauncherOperationUiModel uiModel = LauncherUiModelForOperation("workspace.build");
		passed &= Expect(uiModel.DisplayName == "Build Workspace", "The composite Build intent must have a reviewer-facing name.");
		return passed;
	}

	static bool CookUsesOneSelectableWorkspaceIntent()
	{
		bool passed = true;
		const QVector<LauncherWorkflowDefinition> workflows = CreateLauncherWorkflowCatalog();
		const auto cook = std::find_if(
		    workflows.begin(),
		    workflows.end(),
		    [](const LauncherWorkflowDefinition& workflow) { return workflow.PageKind == LauncherWorkflowPageKind::Cook; });
		passed &= Expect(cook != workflows.end(), "The frontend workflow catalog must contain Cook.");
		if (cook != workflows.end())
		{
			passed &= Expect(
			    cook->OperationIds == QVector<QString>{"cook.workspace"},
			    "Cook must project one selectable workspace cook intent instead of backend operation tabs.");
		}

		const std::optional<CookOperationDefinition> definition = FindCookOperationDefinition("cook.workspace");
		passed &= Expect(definition.has_value(), "CookOperations must register cook.workspace.");
		passed &= Expect(
		    definition.has_value() && definition->Kind == CookOperationKind::CookWorkspace,
		    "cook.workspace must use the composite CookOperations planner.");
		passed &= Expect(
		    FindCookOperationDefinition("cook.all").has_value() && FindCookOperationDefinition("cook.shaders").has_value()
		        && FindCookOperationDefinition("cook.textures").has_value() && FindCookOperationDefinition("cook.assets").has_value(),
		    "Quick Start cook capability primitives must remain registered below the frontend projection.");

		const LauncherOperationUiModel uiModel = LauncherUiModelForOperation("cook.workspace");
		passed &= Expect(uiModel.DisplayName == "Cook Workspace", "The composite Cook intent must have a reviewer-facing name.");
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
	if (!BuildUsesOneSelectableWorkspaceIntent())
	{
		return 1;
	}
	if (!CookUsesOneSelectableWorkspaceIntent())
	{
		return 1;
	}

	std::cout << "Launcher workflow domain/projection agreement passed.\n";
	return 0;
}
