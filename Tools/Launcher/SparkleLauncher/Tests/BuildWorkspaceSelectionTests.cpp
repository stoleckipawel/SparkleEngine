#include "BuildWorkspaceProcessRequests.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

namespace SparkleLauncher::BuildWorkspaceSelectionTests
{
	static bool Expect(bool condition, const char* message)
	{
		if (!condition)
		{
			std::cerr << message << '\n';
		}
		return condition;
	}

	static bool ContainsArgument(const ProcessRequest& request, const std::string& value)
	{
		return std::find(request.Arguments.begin(), request.Arguments.end(), value) != request.Arguments.end();
	}

	static bool SelectedScopesAreGroupedByBuildProfile()
	{
		BuildWorkspaceOperationPlan plan;
		plan.Kind = BuildWorkspaceOperationKind::BuildWorkspace;
		plan.Operation.Id = "workspace.build";
		plan.RepositoryRoot = "C:/Sparkle";
		plan.Toolchain.RequiredToolsAvailable = true;
		plan.Toolchain.CMakePath = "C:/CMake/bin/cmake.exe";
		plan.Request.RepositoryRoot = plan.RepositoryRoot;
		plan.Request.ContentId = "Showcase";
		plan.Request.EditorProfile = "DevelopmentEditor";
		plan.Request.RuntimeProfile = "DevelopmentGame";
		plan.Request.SelectedScopes = {BuildWorkspaceScope::Editor, BuildWorkspaceScope::Runtime, BuildWorkspaceScope::Launcher};
		plan.Freshness.Current = true;

		const std::vector<BuildWorkspaceProcessStep> steps = BuildProcessStepsForPlan(plan);
		bool passed = true;
		passed &= Expect(steps.size() == 2, "A current workspace must group selected products into one build per profile.");
		if (steps.size() == 2)
		{
			passed &= Expect(
			    ContainsArgument(steps[0].Request, "SparkleLauncher") && ContainsArgument(steps[0].Request, "ShowcaseEditor"),
			    "Launcher and editor targets must share the editor-profile build request.");
			passed &= Expect(
			    ContainsArgument(steps[1].Request, "ShowcaseRuntime"),
			    "The runtime target must use the separate runtime-profile build request.");
		}

		plan.Freshness.Current = false;
		const std::vector<BuildWorkspaceProcessStep> staleSteps = BuildProcessStepsForPlan(plan);
		passed &= Expect(
		    staleSteps.size() == 3 && staleSteps.front().Id == "configure",
		    "A stale workspace must infer one configure step before the selected product builds.");
		return passed;
	}
}

int main()
{
	using namespace SparkleLauncher::BuildWorkspaceSelectionTests;
	if (!SelectedScopesAreGroupedByBuildProfile())
	{
		return 1;
	}

	std::cout << "Build workspace selection contract passed.\n";
	return 0;
}
