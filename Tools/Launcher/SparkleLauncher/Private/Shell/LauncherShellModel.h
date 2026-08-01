#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	struct LauncherShellArguments;

	struct LauncherShellOperationRow final
	{
		std::string Group;
		std::string Id;
		std::string DisplayName;
		std::string Readiness;
		std::string NextEffect;
	};

	struct LauncherShellActivityEntry final
	{
		std::string TimeText;
		std::string Summary;
	};

	struct LauncherShellModel final
	{
		RepositoryRoot Repository;
		std::vector<SparkleProject> Projects;
		std::string SelectedProjectId;
		std::string EditorProfile = "DevelopmentEditor";
		std::string RuntimeProfile = "DevelopmentGame";
		WorkspaceIde WorkspaceIdePreference = WorkspaceIde::VisualStudio;
		std::vector<LauncherShellOperationRow> Operations;
		std::vector<LauncherShellActivityEntry> Activity;
		std::vector<std::string> JobOutput;
	};

	LauncherShellModel BuildLauncherShellModel(
	    RepositoryRoot repository,
	    std::vector<SparkleProject> projects,
	    const LauncherShellArguments& arguments);
	const LauncherShellOperationRow* FindLauncherShellOperation(const LauncherShellModel& model, std::string_view operationId) noexcept;
	void RecordLauncherShellActivity(LauncherShellModel& model, std::string summary);
}
