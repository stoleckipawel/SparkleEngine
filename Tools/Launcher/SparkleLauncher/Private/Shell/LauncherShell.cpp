#include "LauncherShell.h"

#include "LauncherShellArguments.h"
#include "LauncherShellModel.h"
#include "LauncherShellOperations.h"
#include "LauncherShellPresentation.h"

#include "SparkleLauncher/ProjectDiscovery.h"
#include "SparkleLauncher/RepositoryLocator.h"

#include <filesystem>
#include <optional>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	int LauncherShell::Run(int argc, char** argv, std::ostream& output, std::ostream& error) const
	{
		LauncherShellArguments arguments;
		if (!ParseLauncherShellArguments(argc, argv, arguments, error))
		{
			PrintLauncherShellUsage(error);
			return 1;
		}

		if (arguments.ShowHelp)
		{
			PrintLauncherShellUsage(output);
			return 0;
		}

		std::string errorMessage;
		const std::filesystem::path startPath = arguments.StartPath.empty() ? std::filesystem::current_path() : arguments.StartPath;
		const std::optional<RepositoryRoot> repository = TryFindRepositoryRoot(startPath, errorMessage);
		if (!repository.has_value())
		{
			error << errorMessage << '\n';
			return 1;
		}

		std::vector<SparkleProject> projects = DiscoverProjects(repository->RootPath, errorMessage);
		if (!errorMessage.empty())
		{
			error << errorMessage << '\n';
			return 1;
		}

		LauncherShellModel model = BuildLauncherShellModel(*repository, std::move(projects), arguments);
		if (!arguments.RunOperationId.empty())
		{
			return RunLauncherShellOperation(model, arguments, output, error);
		}
		if (!arguments.DryRunOperationId.empty())
		{
			ApplyLauncherShellDryRun(model, arguments);
		}

		RenderLauncherShell(model, output);
		return 0;
	}
}
