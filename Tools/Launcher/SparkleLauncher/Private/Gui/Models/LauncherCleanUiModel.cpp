#include "LauncherCleanUiModel.h"

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QRegularExpression>

namespace SparkleLauncher
{
	std::filesystem::path ResolveCleanScopePreviewPath(
	    const std::filesystem::path& repositoryRoot,
	    const QString& projectId,
	    const QString& scope)
	{
		if (scope == "cooked")
		{
			return GetCookedProjectDirectory(repositoryRoot, projectId.toStdString());
		}
		if (scope == "build-tree")
		{
			return GetBuildDirectory(repositoryRoot);
		}
		if (scope == "artifacts")
		{
			return GetArtifactDirectory(repositoryRoot);
		}
		if (scope == "workspace-state")
		{
			return repositoryRoot;
		}
		if (scope == "deps")
		{
			return GetBuildDirectory(repositoryRoot) / "_deps";
		}
		if (scope == "logs")
		{
			return repositoryRoot / "logs";
		}
		return repositoryRoot;
	}

	QString CleanScopeDisplayName(const QString& scopeValue)
	{
		if (scopeValue == "cooked")
		{
			return "Cooked Content";
		}
		if (scopeValue == "build-tree")
		{
			return "Build Outputs";
		}
		if (scopeValue == "artifacts")
		{
			return "Generated Artifacts";
		}
		if (scopeValue == "workspace-state")
		{
			return "IDE And Workspace State";
		}
		if (scopeValue == "deps")
		{
			return "Source Dependency Cache";
		}
		if (scopeValue == "logs")
		{
			return "Log Files";
		}
		if (scopeValue == "clean-all")
		{
			return "Clean All";
		}
		return scopeValue;
	}

	bool SupportsActionSpecificClean(const QString& operationId)
	{
		return operationId == "launcher.build.self" || operationId.startsWith("workspace.build") || operationId == "cook.tools.prepare"
		    || operationId == "cook.workspace" || operationId == "cook.all" || operationId == "cook.shaders"
		    || operationId == "cook.textures" || operationId == "cook.assets";
	}

	void AddExplicitCleanTarget(
	    QVector<LauncherCleanTarget>& targets,
	    const QString& displayName,
	    const std::filesystem::path& path,
	    const QString& detail)
	{
		LauncherCleanTarget target;
		target.DisplayName = displayName;
		target.Path = QString::fromStdString(path.string());
		target.Detail = detail;
		targets.push_back(std::move(target));
	}

	void AddTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& targetName,
	    const QString& detail,
	    const std::filesystem::path& preservedPath)
	{
		std::filesystem::path binaryDirectory =
		    GetDeveloperArtifactDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		std::filesystem::path libraryDirectory =
		    GetDeveloperLibraryDirectory(repositoryRoot, "runtime-support/" + targetName.toStdString(), profileName.toStdString());
		std::filesystem::path symbolDirectory =
		    GetSymbolDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		if (targetName == "SparkleLauncher")
		{
			binaryDirectory = GetLauncherArtifactDirectory(repositoryRoot, profileName.toStdString());
			libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "launcher", profileName.toStdString());
			symbolDirectory = GetSymbolDirectory(repositoryRoot) / "launcher" / profileName.toStdString();
		}
		else if (targetName == "AssetCooker" || targetName == "TextureCooker" || targetName == "ShaderCompiler")
		{
			binaryDirectory = GetDevelopmentToolArtifactDirectory(repositoryRoot, targetName.toStdString(), profileName.toStdString());
			libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "tools/" + targetName.toStdString(), profileName.toStdString());
			symbolDirectory = GetSymbolDirectory(repositoryRoot) / "tools" / targetName.toStdString() / profileName.toStdString();
		}
		const std::filesystem::path executablePath = binaryDirectory / (targetName.toStdString() + ".exe");
		if (preservedPath.empty() || executablePath != preservedPath)
		{
			AddExplicitCleanTarget(targets, targetName + " executable", executablePath, detail);
		}
		AddExplicitCleanTarget(targets, targetName + " program database", symbolDirectory / (targetName.toStdString() + ".pdb"), detail);
		AddExplicitCleanTarget(targets, targetName + " import library", libraryDirectory / (targetName.toStdString() + ".lib"), detail);
		AddExplicitCleanTarget(
		    targets,
		    targetName + " compile database",
		    symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"),
		    detail);
	}

	void AddProjectTargetArtifactOutputs(
	    QVector<LauncherCleanTarget>& targets,
	    const std::filesystem::path& repositoryRoot,
	    const QString& profileName,
	    const QString& projectName,
	    const QString& productRole,
	    const QString& targetName,
	    const QString& detail)
	{
		const std::filesystem::path binaryDirectory = GetProjectTargetArtifactDirectory(
		    repositoryRoot,
		    projectName.toStdString(),
		    productRole.toStdString(),
		    profileName.toStdString());
		const std::filesystem::path libraryDirectory = GetDeveloperLibraryDirectory(
		    repositoryRoot,
		    "projects/" + projectName.toStdString() + "/" + productRole.toStdString(),
		    profileName.toStdString());
		const std::filesystem::path symbolDirectory = GetSymbolDirectory(repositoryRoot) / "projects" / projectName.toStdString()
		    / productRole.toStdString() / profileName.toStdString();
		const std::filesystem::path executablePath = binaryDirectory / (targetName.toStdString() + ".exe");
		AddExplicitCleanTarget(targets, targetName + " executable", executablePath, detail);
		AddExplicitCleanTarget(targets, targetName + " program database", symbolDirectory / (targetName.toStdString() + ".pdb"), detail);
		AddExplicitCleanTarget(targets, targetName + " import library", libraryDirectory / (targetName.toStdString() + ".lib"), detail);
		AddExplicitCleanTarget(
		    targets,
		    targetName + " compile database",
		    symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"),
		    detail);
	}

	QVector<LauncherCleanTarget> BuildActionSpecificCleanTargets(const ActionCleanTargetContext& context)
	{
		QVector<LauncherCleanTarget> targets;
		const QStringList buildScopes = context.BuildScopes.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
		const QStringList cookScopes = context.CookScopes.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts);
		const bool workspaceBuild = context.OperationId == "workspace.build";
		const bool buildsContentProduct = workspaceBuild && (buildScopes.contains("editor") || buildScopes.contains("runtime"));
		if ((buildsContentProduct || context.OperationId == "workspace.build.editor" || context.OperationId == "workspace.build.runtime"
		        || context.OperationId.startsWith("cook."))
		    && context.ContentId.isEmpty())
		{
			return targets;
		}

		const auto addNamedTargets = [&context, &targets](const QString& profileName, const QStringList& targetNames, const QString& detail)
		{
			for (const QString& targetName : targetNames)
			{
				if (!targetName.isEmpty())
				{
					AddTargetArtifactOutputs(targets, context.RepositoryRoot, profileName, targetName, detail);
				}
			}
		};
		const auto addProjectArtifacts =
		    [&context, &addNamedTargets, &targets](const QString& profileName, const QString& projectName, const QString& detail)
		{
			const std::optional<BuildProfile> profile = FindBuildProfile(profileName.toStdString());
			if (!profile.has_value())
			{
				return;
			}
			const QString productRole = profile->Target == BuildProfileTarget::Game ? "runtime" : "editor";
			AddProjectTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    profileName,
			    projectName,
			    productRole,
			    QString::fromStdString(BuildProjectTargetName(projectName.toStdString(), *profile)),
			    detail);
		};

		if (context.OperationId == "launcher.build.self" || (workspaceBuild && buildScopes.contains("launcher")))
		{
			AddTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    context.EditorProfile,
			    "SparkleLauncher",
			    "Launcher direct build outputs. The currently running launcher executable is preserved until restart.",
			    context.RunningLauncherPath);
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore library",
			    GetSymbolDirectory(context.RepositoryRoot) / "launcher" / context.EditorProfile.toStdString() / "lib"
			        / "SparkleLauncherCore.lib",
			    "Launcher support library built for the selected editor profile.");
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore program database",
			    GetSymbolDirectory(context.RepositoryRoot) / "launcher" / context.EditorProfile.toStdString() / "lib"
			        / "SparkleLauncherCore.pdb",
			    "Launcher support library debug symbols built for the selected editor profile.");
		}

		if (context.OperationId == "workspace.build.editor" || (workspaceBuild && buildScopes.contains("editor")))
		{
			if (context.OperationId == "workspace.build.editor" && !context.SelectedTargets.trimmed().isEmpty())
			{
				addNamedTargets(
				    context.EditorProfile,
				    context.SelectedTargets.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts),
				    "Selected editor build target outputs.");
			}
			else
			{
				addProjectArtifacts(context.EditorProfile, context.ContentId, "Editor target outputs.");
			}
		}

		if (context.OperationId == "workspace.build.runtime" || (workspaceBuild && buildScopes.contains("runtime")))
		{
			if (context.OperationId == "workspace.build.runtime" && !context.SelectedTargets.trimmed().isEmpty())
			{
				addNamedTargets(
				    context.RuntimeProfile,
				    context.SelectedTargets.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts),
				    "Selected runtime build target outputs.");
			}
			else
			{
				addProjectArtifacts(context.RuntimeProfile, context.ContentId, "Runtime target outputs.");
			}
		}

		if (context.OperationId == "cook.tools.prepare" || (workspaceBuild && buildScopes.contains("cook-tools")))
		{
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    context.EditorProfile,
			    "AssetCooker",
			    "AssetCooker executable outputs.");
			AddTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    context.EditorProfile,
			    "TextureCooker",
			    "TextureCooker executable outputs.");
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
			AddTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    context.EditorProfile,
			    "ShaderCompiler",
			    "ShaderCompiler executable outputs.");
#endif
		}

		if (context.OperationId == "cook.workspace")
		{
			if (cookScopes.contains("textures") || cookScopes.contains("assets"))
			{
				AddExplicitCleanTarget(
				    targets,
				    "Cooked content",
				    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()),
				    "Generated texture, scene, mesh, and material outputs selected by Cook Workspace.");
			}
			if (cookScopes.contains("shaders"))
			{
				AddExplicitCleanTarget(
				    targets,
				    "Cooked shaders",
				    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()) / "Shaders",
				    "Generated shader outputs selected by Cook Workspace.");
			}
		}
		else if (context.OperationId == "cook.all")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked content",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()),
			    "All generated cooked content.");
			AddExplicitCleanTarget(
			    targets,
			    "Cooked shaders",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()) / "Shaders",
			    "Generated shader outputs refreshed by cook operations.");
		}
		else if (context.OperationId == "cook.shaders")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked shaders",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()) / "Shaders",
			    "Generated shader outputs refreshed by shader cooking.");
		}
		else if (context.OperationId == "cook.textures" || context.OperationId == "cook.assets")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked content",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ContentId.toStdString()),
			    context.OperationId == "cook.textures" ? "Cooked texture outputs." : "Cooked mesh and material outputs.");
		}

		return targets;
	}
}
