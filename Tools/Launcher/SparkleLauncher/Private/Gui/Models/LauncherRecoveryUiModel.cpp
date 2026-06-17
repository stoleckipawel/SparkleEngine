#include "LauncherRecoveryUiModel.h"

#include <QtCore/Qt>

namespace SparkleLauncher
{
	LauncherRecoveryAction RecoveryActionForFailure(const QString& operationId, const QString& statusText)
	{
		if (statusText.contains("generator platform", Qt::CaseInsensitive) || statusText.contains("CMakeCache", Qt::CaseInsensitive) ||
		    statusText.contains("Generate Workspace", Qt::CaseInsensitive) || statusText.contains("Generated workspace", Qt::CaseInsensitive))
		{
			return {"workspace.generate-build-files", "Generate Build Files", "Refresh generated CMake and IDE files for the selected toolchain.", true};
		}
		if (statusText.contains("tool", Qt::CaseInsensitive) || statusText.contains("MSBuild", Qt::CaseInsensitive) || statusText.contains("Visual Studio", Qt::CaseInsensitive) ||
		    statusText.contains("Qt", Qt::CaseInsensitive))
		{
			return {"workspace.sync-source-tiers", "Open Sync", "Inspect machine readiness and repository dependency status.", true};
		}
		if (operationId == "project.open.editor" || (operationId == "project.run" && statusText.contains("Editor", Qt::CaseInsensitive)))
		{
			return {"project.build.editor", "Build Editor", "Rebuild the selected project editor artifact.", true};
		}
		if (operationId == "project.open.runtime" || statusText.contains("Runtime", Qt::CaseInsensitive))
		{
			return {"project.build.runtime", "Build Runtime", "Rebuild the selected project runtime artifact.", true};
		}
		if (statusText.contains("scene", Qt::CaseInsensitive) || statusText.contains("mesh", Qt::CaseInsensitive))
		{
			return {"cook.assets", "Cook Scenes And Meshes", "Refresh scene and mesh cooked outputs.", true};
		}
		if (statusText.contains("texture", Qt::CaseInsensitive))
		{
			return {"cook.textures", "Cook Textures", "Refresh texture cooked outputs.", true};
		}
		if (statusText.contains("shader", Qt::CaseInsensitive))
		{
			return {"cook.shaders", "Cook Shaders", "Refresh shader cooked outputs.", true};
		}
		if (operationId.startsWith("cook."))
		{
			return {"cook.tools.prepare", "Build Cooking Tools", "Build local cook tool outputs before retrying cook workflows.", true};
		}
		if (operationId.startsWith("project.build") || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId == "cook.tools.prepare")
		{
			return {"workspace.sync-source-tiers", "Open Sync", "Inspect machine readiness before retrying the build.", true};
		}
		return {};
	}
}
