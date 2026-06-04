#include "LauncherWorkflowCatalog.h"

namespace SparkleLauncher
{
	QString LauncherHomeOperationId()
	{
		return "home.quick-start";
	}

	QString LauncherSystemOperationId()
	{
		return "system.overview";
	}

	QVector<LauncherWorkflowDefinition> CreateLauncherWorkflowCatalog()
	{
		return {
		    {"Home", "First-contact command center", {LauncherHomeOperationId()}, "home"},
		    {"Launch", "Open what is ready", {"project.run", "workspace.open-solution"}, "launch"},
		    {"Prepare", "Make source work ready", {"toolchain.check", "workspace.setup", "workspace.generate-solution"}, "prepare"},
		    {"Build", "Optional local rebuilds", {"workspace.build-all", "launcher.build.self", "project.build.editor", "project.build.runtime", "cook.tools.prepare"}, "build"},
		    {"Cook", "Optional content refresh", {"cook.project", "cook.shaders", "cook.textures", "cook.assets"}, "cook"},
		    {"Validate", "Test runtime confidence", {"project.run.smoke"}, "validate"},
		    {"Package", "Release assembly", {"package.release"}, "package"},
		    {"System", "Workspace and machine state", {LauncherSystemOperationId()}, "system"},
		    {"Maintain", "Clean and format", {"workspace.clean", "quality.format"}, "maintain"},
		};
	}

	QString LauncherOperationDisplayNameOverride(const QString& operationId)
	{
		if (operationId == LauncherHomeOperationId())
		{
			return "Command Center";
		}
		if (operationId == LauncherSystemOperationId())
		{
			return "System";
		}
		if (operationId == "workspace.open-solution")
		{
			return "Open IDE";
		}
		if (operationId == "project.run")
		{
			return "Launch Project";
		}
		if (operationId == "package.release")
		{
			return "Assemble Release Package";
		}
		return {};
	}

	QString LauncherOperationImpactText(const QString& operationId)
	{
		if (operationId == "toolchain.check")
		{
			return "Diagnostics only: audits installed host prerequisites and does not modify workspace dependencies or outputs.";
		}
		if (operationId == "workspace.setup")
		{
			return "Source tiers: populates enabled workspace source tiers and configure state; it does not install host tools.";
		}
		if (operationId == "workspace.generate-solution")
		{
			return "Workspace files: refreshes generated CMake and IDE state without building products.";
		}
		if (operationId == "workspace.open-solution")
		{
			return "Navigation only: opens the selected IDE once generated project files are current.";
		}
		if (operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build") || operationId == "cook.tools.prepare")
		{
			return "Build outputs: optional local rebuild that can replace ready-to-use bundled binaries for development work.";
		}
		if (operationId.startsWith("cook."))
		{
			return "Cooked outputs: optional local recook that refreshes generated project content.";
		}
		if (operationId == "project.open.editor" || operationId == "project.open.runtime")
		{
			return "Product shortcut: opens the selected editor or runtime directly from Home when ready.";
		}
		if (operationId == "project.run")
		{
			return "Launch workflow: runs the selected editor or runtime target with shared graphics and argument options.";
		}
		if (operationId == "project.run.smoke")
		{
			return "Validate workflow: runs the selected target with smoke validation enabled.";
		}
		if (operationId == "package.release")
		{
			return "Package outputs: assembles runtime and symbols packages from artifacts into dist/releases/<version>; publishing and release sign-off stay separate.";
		}
		if (operationId == "workspace.clean")
		{
			return "Maintain: removes selected generated outputs, caches, logs, or local workspace state after confirmation.";
		}
		if (operationId == "quality.format")
		{
			return "Maintain: formats or checks source files; it does not build, cook, or sync dependencies.";
		}
		return {};
	}

	QString LauncherWorkflowPrimaryVerb(const QString& operationId)
	{
		if (operationId == "toolchain.check")
		{
			return "Audit";
		}
		if (operationId == "workspace.setup")
		{
			return "Sync";
		}
		if (operationId == "workspace.generate-solution")
		{
			return "Generate";
		}
		if (operationId == "workspace.open-solution")
		{
			return "Open";
		}
		if (operationId.startsWith("project.open.") || operationId == "project.run")
		{
			return "Launch";
		}
		if (operationId.startsWith("project.run."))
		{
			return "Validate";
		}
		if (operationId.startsWith("project.build") || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId == "cook.tools.prepare")
		{
			return "Build";
		}
		if (operationId.startsWith("cook."))
		{
			return "Cook";
		}
		if (operationId == "package.release")
		{
			return "Assemble";
		}
		if (operationId == "workspace.clean")
		{
			return "Clean";
		}
		if (operationId == "quality.format")
		{
			return "Format";
		}
		return "Run";
	}
}
