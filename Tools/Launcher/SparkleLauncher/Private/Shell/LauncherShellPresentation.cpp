#include "LauncherShellPresentation.h"

#include "LauncherShellModel.h"

#include "SparkleLauncher/BuildProfileCatalog.h"

#include <ostream>
#include <string>
#include <string_view>

namespace SparkleLauncher
{
	std::string BuildProfileOptionText(BuildProfileTarget target)
	{
		std::string text;
		for (const BuildProfile& profile : GetBuildProfileCatalog())
		{
			if (profile.Target != target)
			{
				continue;
			}

			if (!text.empty())
			{
				text += ", ";
			}
			text += profile.Name;
		}
		return text;
	}

	void RenderLauncherOperationGroup(const LauncherShellModel& model, std::string_view groupName, std::ostream& output)
	{
		output << groupName << "\n";
		for (const LauncherShellOperationRow& operation : model.Operations)
		{
			if (operation.Group == groupName)
			{
				output << "  " << operation.DisplayName << " [" << operation.Readiness << "] " << operation.NextEffect << '\n';
			}
		}
	}

	void RenderLauncherShell(const LauncherShellModel& model, std::ostream& output)
	{
		output << "Sparkle Launcher\n";
		output << "Repository: " << model.Repository.RootPath.string() << "\n\n";
		output << "Profile selectors\n";
		output << "  Editor: " << model.EditorProfile << " | options: " << BuildProfileOptionText(BuildProfileTarget::Editor) << '\n';
		output << "  Runtime/Cook: " << model.RuntimeProfile << " | options: " << BuildProfileOptionText(BuildProfileTarget::Game) << '\n';
		output << "  Workspace IDE: " << DisplayName(model.WorkspaceIdePreference) << " | options: Visual Studio, Rider\n\n";

		RenderLauncherOperationGroup(model, "Launch", output);
		RenderLauncherOperationGroup(model, "Sync", output);
		RenderLauncherOperationGroup(model, "Build", output);
		RenderLauncherOperationGroup(model, "Cook", output);
		RenderLauncherOperationGroup(model, "Clean", output);

		output << "\nRecent Activity\n";
		for (const LauncherShellActivityEntry& entry : model.Activity)
		{
			output << "  " << entry.TimeText << "  " << entry.Summary << '\n';
		}

		output << "\nJob Output\n";
		if (model.JobOutput.empty())
		{
			output << "  No active job. Use --dry-run [operation-id] to preview an operation.\n";
			return;
		}

		for (const std::string& line : model.JobOutput)
		{
			output << "  " << line << '\n';
		}
	}
}
