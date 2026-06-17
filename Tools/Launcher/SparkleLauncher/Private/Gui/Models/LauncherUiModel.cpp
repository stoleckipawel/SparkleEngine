#include "LauncherUiModel.h"

namespace SparkleLauncher
{
	namespace
	{
		LauncherOperationUiModel MakeOperationUiModel(
		    const QString& operationId,
		    const QString& displayName,
		    const QString& primaryVerb,
		    LauncherWorkflowPageKind pageKind,
		    LauncherActionImpactKind impactKind,
		    const QString& impactText,
		    const QString& visualTitle,
		    const QString& visualText,
		    const QString& visualAssetName)
		{
			LauncherOperationUiModel model;
			model.OperationId = operationId;
			model.DisplayName = displayName;
			model.PrimaryVerb = primaryVerb;
			model.PageKind = pageKind;
			model.ImpactKind = impactKind;
			model.ImpactText = impactText;
			model.VisualTitle = visualTitle;
			model.VisualText = visualText;
			model.VisualAssetName = visualAssetName;
			return model;
		}

		LauncherOperationUiModel UnknownOperationUiModel(const QString& operationId)
		{
			LauncherOperationUiModel model;
			model.OperationId = operationId;
			model.PrimaryVerb = "Run";
			model.VisualTitle = "Workflow";
			model.VisualText = "Context artwork is informational; the primary action remains in the workflow controls below.";
			model.VisualAssetName = "workflow-fallback-tools.png";
			return model;
		}
	}

	LauncherOperationUiModel LauncherUiModelForOperation(const QString& operationId)
	{
		if (operationId == "home.quick-start")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Quick Start",
			    "Quick Start",
			    LauncherWorkflowPageKind::Home,
			    LauncherActionImpactKind::None,
			    {},
			    "Explore Project",
			    "Choose Editor or Runtime below. Products use the Launch Project settings, including startup level, and launch from local source artifacts when available.",
			    "workflow-home-quickstart.png");
		}
		if (operationId == "project.open.editor")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Open Editor",
			    "Open",
			    LauncherWorkflowPageKind::Launch,
			    LauncherActionImpactKind::LaunchProcess,
			    "Product shortcut: opens the selected editor directly from Quick Start when ready.",
			    "Launch view",
			    "Uses the selected project, target, startup level, and runtime options from the Launch Project page.",
			    "workflow-editor-open.png");
		}
		if (operationId == "project.open.runtime")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Open Runtime",
			    "Open",
			    LauncherWorkflowPageKind::Launch,
			    LauncherActionImpactKind::LaunchProcess,
			    "Product shortcut: opens the selected runtime directly from Quick Start when ready.",
			    "Launch view",
			    "Uses the selected project, target, startup level, and runtime options from the Launch Project page.",
			    "workflow-runtime-open.png");
		}
		if (operationId == "project.run")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Launch Project",
			    "Run",
			    LauncherWorkflowPageKind::Launch,
			    LauncherActionImpactKind::LaunchProcess,
			    "Launch workflow: runs the selected editor or runtime target with shared graphics, startup level, and argument options.",
			    "Launch view",
			    "Uses the selected project, target, startup level, and runtime options from this page.",
			    "workflow-project-run-editor.png");
		}
		if (operationId == "workspace.open-ide")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Open IDE",
			    "Open",
			    LauncherWorkflowPageKind::Launch,
			    LauncherActionImpactKind::LaunchProcess,
			    "Navigation only: opens the selected IDE once generated project files are current.",
			    "Workspace files",
			    "Refreshes project files for the selected toolchain and opens the IDE when the workspace is current.",
			    "workflow-open-ide.png");
		}
		if (operationId == "toolchain.check")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Verify Host Environment",
			    "Check",
			    LauncherWorkflowPageKind::Sync,
			    LauncherActionImpactKind::Diagnostics,
			    "Diagnostics only: audits installed host prerequisites and does not modify workspace dependencies or outputs.",
			    "Host readiness",
			    "Checks installed tools without changing source dependencies, artifacts, or cooked outputs.",
			    "workflow-toolchain-check.png");
		}
		if (operationId == "workspace.sync-source-tiers")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Prepare Workspace",
			    "Sync",
			    LauncherWorkflowPageKind::Sync,
			    LauncherActionImpactKind::SourceDependencies,
			    "Downloads and repairs enabled repository dependencies, then refreshes workspace configure state; it does not install host tools.",
			    "Workspace setup",
			    "Fetch the repository packages needed for local builds, content tools, and optional renderer integrations.",
			    "workflow-source-sync.png");
		}
		if (operationId == "workspace.generate-build-files")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Generate Build Files",
			    "Generate",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::WorkspaceFiles,
			    "Build files: refreshes generated CMake and IDE build-system state without building products.",
			    "Workspace files",
			    "Refreshes project files for the selected toolchain and opens the IDE when the workspace is current.",
			    "workflow-generate-build-files.png");
		}
		if (operationId == "workspace.build-all")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build All",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild that can replace ready-to-use bundled binaries for development work.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-build-all.png");
		}
		if (operationId == "launcher.build.self")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Launcher",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild that refreshes the launcher artifact for development work.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-launcher-build.png");
		}
		if (operationId == "project.build.editor")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Editor",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild that can replace ready-to-use bundled editor binaries for development work.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-editor-build.png");
		}
		if (operationId == "project.build.runtime")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Runtime",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild that can replace ready-to-use bundled runtime binaries for development work.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-runtime-build.png");
		}
		if (operationId == "cook.tools.prepare")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Cook Tools",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: prepares local cook tools for optional content refresh workflows.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-cook-tools.png");
		}
		if (operationId == "cook.project")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook All",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes all generated project content.",
			    "Content outputs",
			    "Refreshes cooked content for the selected project and startup level.",
			    "workflow-cook-all.png");
		}
		if (operationId == "cook.shaders")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook Shaders",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes generated shader content.",
			    "Content outputs",
			    "Refreshes cooked content for the selected project and startup level.",
			    "workflow-cook-shaders.png");
		}
		if (operationId == "cook.textures")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook Textures",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes generated texture content.",
			    "Content outputs",
			    "Refreshes cooked content for the selected project and startup level.",
			    "workflow-cook-textures.png");
		}
		if (operationId == "cook.assets")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook Scene Assets",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes generated scene content.",
			    "Content outputs",
			    "Refreshes cooked content for the selected project and startup level.",
			    "workflow-cook-assets.png");
		}
		if (operationId == "project.run.smoke")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Run Smoke Test",
			    "Run",
			    LauncherWorkflowPageKind::Test,
			    LauncherActionImpactKind::TestRun,
			    "Test workflow: runs one smoke entry point, then optionally executes selected PTLAS parity, benchmark, and diagnostic capture matrices.",
			    "Validation run",
			    "Runs a focused confidence check using the same launch parameters as the product path, with optional capability-driven PTLAS diagnostics.",
			    "workflow-smoke-test.png");
		}
		if (operationId == "quality.format")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Format Check",
			    "Run",
			    LauncherWorkflowPageKind::Test,
			    LauncherActionImpactKind::Diagnostics,
			    "Quality gate: checks or applies clang-format for source files; it does not build, cook, or sync dependencies.",
			    "Code quality",
			    "Applies source formatting while leaving build artifacts and cooked content alone.",
			    "workflow-format-check.png");
		}
		if (operationId == "package.release")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Assemble Release Package",
			    "Assemble",
			    LauncherWorkflowPageKind::Package,
			    LauncherActionImpactKind::PackageOutputs,
			    "Package outputs: assembles runtime and symbols packages from artifacts into dist/releases/<version>; publishing and release sign-off stay separate.",
			    "Release assembly",
			    "Stages reviewable release packages from artifacts and manifests; publishing remains a separate sign-off.",
			    "workflow-package-release.png");
		}
		if (operationId == "workspace.clean")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Clean Workspace",
			    "Clean",
			    LauncherWorkflowPageKind::Clean,
			    LauncherActionImpactKind::GeneratedState,
			    "Clean: removes selected generated outputs, caches, logs, or local workspace state after confirmation.",
			    "Generated files",
			    "Shows what generated state will be removed and what will stay before destructive cleanup.",
			    "workflow-clean-workspace.png");
		}

		if (operationId.startsWith("project.build."))
		{
			return MakeOperationUiModel(
			    operationId,
			    {},
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild that can replace ready-to-use bundled binaries for development work.",
			    "Build outputs",
			    "Creates local artifacts that can replace packaged binaries during daily development.",
			    "workflow-build-generic.png");
		}
		if (operationId.startsWith("cook."))
		{
			return MakeOperationUiModel(
			    operationId,
			    {},
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes generated project content.",
			    "Content outputs",
			    "Refreshes cooked content for the selected project and startup level.",
			    "workflow-cook-generic.png");
		}

		return UnknownOperationUiModel(operationId);
	}

	QString LauncherWorkflowPageKindName(LauncherWorkflowPageKind pageKind)
	{
		switch (pageKind)
		{
		case LauncherWorkflowPageKind::Home:
			return "Quick Start";
		case LauncherWorkflowPageKind::Launch:
			return "Launch";
		case LauncherWorkflowPageKind::Sync:
			return "Sync";
		case LauncherWorkflowPageKind::Build:
			return "Build";
		case LauncherWorkflowPageKind::Cook:
			return "Cook";
		case LauncherWorkflowPageKind::Test:
			return "Test";
		case LauncherWorkflowPageKind::Package:
			return "Package";
		case LauncherWorkflowPageKind::Clean:
			return "Clean";
		case LauncherWorkflowPageKind::Unknown:
			return "Unknown";
		}
		return "Unknown";
	}

	QString LauncherActionImpactKindName(LauncherActionImpactKind impactKind)
	{
		switch (impactKind)
		{
		case LauncherActionImpactKind::None:
			return "None";
		case LauncherActionImpactKind::Diagnostics:
			return "Diagnostics";
		case LauncherActionImpactKind::SourceDependencies:
			return "Source dependencies";
		case LauncherActionImpactKind::WorkspaceFiles:
			return "Workspace files";
		case LauncherActionImpactKind::BuildOutputs:
			return "Build outputs";
		case LauncherActionImpactKind::CookedOutputs:
			return "Cooked outputs";
		case LauncherActionImpactKind::LaunchProcess:
			return "Launch process";
		case LauncherActionImpactKind::TestRun:
			return "Test run";
		case LauncherActionImpactKind::PackageOutputs:
			return "Package outputs";
		case LauncherActionImpactKind::GeneratedState:
			return "Generated state";
		}
		return "Unknown";
	}
}
