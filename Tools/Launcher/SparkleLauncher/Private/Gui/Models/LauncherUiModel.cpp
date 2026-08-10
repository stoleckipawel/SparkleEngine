#include "LauncherUiModel.h"

namespace SparkleLauncher
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
			    "Explore Sparkle",
			    "Choose a level; Quick Start acquires missing content, prepares build and cook prerequisites, then opens that map using "
			    "the "
			    "selected Run Mode.",
			    "workflow-home-quickstart.png");
		}
		if (operationId == "levels.run")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Run Level",
			    "Run",
			    LauncherWorkflowPageKind::Home,
			    LauncherActionImpactKind::LevelAssets,
			    "Quick Start goal: prepares the selected level and product prerequisites, then opens it using the selected Run Mode.",
			    "Open level",
			    "Uses the level selected from its Quick Start card.",
			    "workflow-home-quickstart.png");
		}
		if (operationId == "workspace.sync-code")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Sync Code",
			    "Sync All",
			    LauncherWorkflowPageKind::Sync,
			    LauncherActionImpactKind::SourceDependencies,
			    "Downloads and repairs enabled repository dependencies, refreshes workspace configure state, and exposes registered "
			    "host-tool "
			    "setup actions.",
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
			    "Refreshes workspace files for the selected toolchain and opens the IDE when the workspace is current.",
			    "workflow-generate-build-files.png");
		}
		if (operationId == "workspace.build")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Workspace",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: incrementally builds the selected products and refreshes generated workspace files when required.",
			    "Selected products",
			    "Choose the editor, game, cooking tools, or launcher, then build them as one dependency-aware request.",
			    "workflow-build-all.png");
		}
		if (operationId == "levels.sync")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Sync Levels",
			    "Sync",
			    LauncherWorkflowPageKind::Home,
			    LauncherActionImpactKind::LevelAssets,
			    "Levels: syncs or cleans maps and their asset packs without changing code or SDK dependencies.",
			    "Levels and assets",
			    "Sync individual levels or the full catalog from declared publisher sources.",
			    "workflow-cook-assets.png");
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
			    "Creates the local launcher artifact used for development.",
			    "workflow-launcher-build.png");
		}
		if (operationId == "workspace.build.editor")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Editor",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild of the editor for development work.",
			    "Build outputs",
			    "Creates the local editor artifact used for development work.",
			    "workflow-editor-build.png");
		}
		if (operationId == "workspace.build.runtime")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Build Runtime",
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild of the runtime for development work.",
			    "Build outputs",
			    "Creates the local runtime artifact used for development work.",
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
			    "Build outputs: prepares local cook tools for asset refresh workflows.",
			    "Build outputs",
			    "Creates the local cooking tools used by Quick Start and manual cook workflows.",
			    "workflow-cook-tools.png");
		}
		if (operationId == "cook.all")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook All",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: optional local recook that refreshes all generated content.",
			    "Content outputs",
			    "Refreshes cooked content for selected catalog levels.",
			    "workflow-cook-all.png");
		}
		if (operationId == "cook.workspace")
		{
			return MakeOperationUiModel(
			    operationId,
			    "Cook Workspace",
			    "Cook",
			    LauncherWorkflowPageKind::Cook,
			    LauncherActionImpactKind::CookedOutputs,
			    "Cooked outputs: incrementally refreshes the selected shader, texture, and scene content.",
			    "Selected content outputs",
			    "Choose shaders, textures, or scene assets, then cook them as one ordered request.",
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
			    "Refreshes cooked shader outputs for selected catalog levels.",
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
			    "Refreshes cooked texture outputs for selected catalog levels.",
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
			    "Refreshes cooked scene outputs for selected catalog levels.",
			    "workflow-cook-assets.png");
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

		if (operationId.startsWith("workspace.build."))
		{
			return MakeOperationUiModel(
			    operationId,
			    {},
			    "Build",
			    LauncherWorkflowPageKind::Build,
			    LauncherActionImpactKind::BuildOutputs,
			    "Build outputs: optional local rebuild of the selected development target.",
			    "Build outputs",
			    "Creates the selected local development artifacts.",
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
			    "Cooked outputs: optional local recook that refreshes generated content.",
			    "Content outputs",
			    "Refreshes cooked content for selected catalog levels.",
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
			case LauncherWorkflowPageKind::Sync:
				return "Sync";
			case LauncherWorkflowPageKind::Build:
				return "Build";
			case LauncherWorkflowPageKind::Cook:
				return "Cook";
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
			case LauncherActionImpactKind::SourceDependencies:
				return "Source dependencies";
			case LauncherActionImpactKind::LevelAssets:
				return "Level assets";
			case LauncherActionImpactKind::WorkspaceFiles:
				return "Workspace files";
			case LauncherActionImpactKind::BuildOutputs:
				return "Build outputs";
			case LauncherActionImpactKind::CookedOutputs:
				return "Cooked outputs";
			case LauncherActionImpactKind::GeneratedState:
				return "Generated state";
		}
		return "Unknown";
	}
}
