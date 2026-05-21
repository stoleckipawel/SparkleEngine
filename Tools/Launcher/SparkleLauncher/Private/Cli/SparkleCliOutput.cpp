#include "SparkleCliOutput.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"

#include <ostream>

namespace SparkleLauncher
{
	void SparkleCliOutput::PrintUsage(std::ostream& output) const
	{
		output << "Usage:\n"
		       << "  Sparkle <operation-id> [--dry-run] [--root <repo-root>] [--project <project-id>] [--editor-profile <profile>] [--runtime-profile <profile>]\n"
		       << "  Sparkle --list-operations\n"
		       << "  Sparkle --list-validation-targets\n"
		       << "\n"
		       << "Options:\n"
		       << "  --target <target>                 Add an explicit build target.\n"
		       << "  --force-configure                 Force CMake configure before build operations.\n"
		       << "  --force-recook                    Use force recook mode for cook operations.\n"
		       << "  --confirm-force-recook            Confirm force recook destructive cleanup.\n"
		       << "  --shader-package <package-id>     Add a focused shader package cook target.\n"
		       << "  --format-mode check|apply         Select clang-format mode.\n"
		       << "  --validation-group <group>        Add a grouped validation set.\n"
		       << "  --validation-target <target>      Add a CMake validation target.\n"
		       << "  --smoke-backend <backend>         Select RHI backend for smoke operations.\n"
		       << "  --smoke-frame-limit <frames>      Set smoke operation frame limit.\n"
		       << "  --smoke-trace                     Enable trace logging for smoke operations.\n"
		       << "  --smoke-skip-level-switching      Disable automated level switching in smoke operations.\n"
		       << "  --clean-scope <scope>             selected-cooked, all-cooked, build-tree, shader-cache, deps, logs, pristine.\n"
		       << "  --confirm-clean                   Confirm destructive clean scope.\n"
		       << "\n"
		       << "Examples:\n"
		       << "  Sparkle workspace.setup --dry-run\n"
		       << "  Sparkle project.build.editor --project Showcase --editor-profile DevelopmentEditor\n"
		       << "  Sparkle smoke.rhi.runtime --project Showcase --smoke-backend d3d12 --dry-run\n"
		       << "  Sparkle cook.shaders --project Showcase --runtime-profile DevelopmentGame --dry-run\n"
		       << "  Sparkle quality.validate --validation-group boundaries --dry-run\n"
		       << "  Sparkle workspace.clean --clean-scope selected-cooked --confirm-clean --dry-run\n";
	}

	void SparkleCliOutput::PrintOperationList(std::ostream& output) const
	{
		for (const BuildWorkspaceOperationDefinition& definition : GetBuildWorkspaceOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const CookOperationDefinition& definition : GetCookOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const MaintenanceOperationDefinition& definition : GetMaintenanceOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
		for (const LaunchOperationDefinition& definition : GetLaunchOperationDefinitions())
		{
			output << definition.Id << " - " << definition.DisplayName << "\n";
		}
	}

	void SparkleCliOutput::PrintValidationTargetList(std::ostream& output) const
	{
		for (const ValidationGateGroupDefinition& group : GetValidationGateGroupDefinitions())
		{
			output << group.Id << " - " << group.DisplayName << "\n";
			output << "  " << group.Description << "\n";
			for (const ValidationGateDefinition& definition : GetValidationGateDefinitions())
			{
				if (definition.GroupId != group.Id)
				{
					continue;
				}
				output << "  " << definition.Target << " - " << definition.DisplayName << "\n";
				output << "    " << definition.Description << "\n";
				output << "    " << definition.Recommendation << "\n";
			}
		}
	}

	void SparkleCliOutput::PrintOperationRecord(const OperationRecord& operation, std::ostream& output) const
	{
		output << operation.DisplayName << " [" << ToString(operation.Status) << "]\n";
		output << "Operation: " << operation.Id << "\n";
		if (!operation.LogPath.empty())
		{
			output << "Log: " << operation.LogPath.string() << "\n";
		}
		if (operation.ExitCode.has_value())
		{
			output << "Exit code: " << *operation.ExitCode << "\n";
		}
		if (!operation.FailureSummary.empty())
		{
			output << "Failure: " << operation.FailureSummary << "\n";
		}
	}

	void SparkleCliOutput::PrintPlanDetails(
	    const OperationRecord& operation,
	    bool canRun,
	    const std::vector<std::string>& readinessMessages,
	    const std::vector<std::string>& plannedEffects,
	    std::ostream& output) const
	{
		output << operation.DisplayName << " [" << (canRun ? "Ready" : "Blocked") << "]\n";
		output << "Operation: " << operation.Id << "\n";
		if (!operation.LogPath.empty())
		{
			output << "Log: " << operation.LogPath.string() << "\n";
		}
		if (operation.RequiresConfirmation)
		{
			output << "Confirmation required: " << ToString(operation.DestructiveScope) << "\n";
		}
		for (const std::string& message : readinessMessages)
		{
			output << "Readiness: " << message << "\n";
		}
		for (const std::string& effect : plannedEffects)
		{
			output << "Effect: " << effect << "\n";
		}
		if (!operation.DryRunText.empty())
		{
			output << operation.DryRunText << "\n";
		}
	}
}