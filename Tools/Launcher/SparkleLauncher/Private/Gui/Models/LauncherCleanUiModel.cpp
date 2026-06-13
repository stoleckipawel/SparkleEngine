#include "LauncherCleanUiModel.h"

#include "SparkleLauncher/BuildProfileCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QRegularExpression>

namespace SparkleLauncher
{
	std::filesystem::path ResolveCleanScopePreviewPath(const std::filesystem::path& repositoryRoot, const QString& projectId, const QString& scope)
	{
		if (scope == "selected-cooked")
		{
			return GetCookedProjectDirectory(repositoryRoot, projectId.toStdString());
		}
		if (scope == "all-cooked")
		{
			return GetCookedProjectsArtifactDirectory(repositoryRoot);
		}
		if (scope == "build-tree")
		{
			return GetBuildDirectory(repositoryRoot);
		}
		if (scope == "shader-cache")
		{
			return GetBuildDirectory(repositoryRoot) / "Cache" / "Shaders";
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
		if (scopeValue == "selected-cooked")
		{
			return "Project Cooked Outputs";
		}
		if (scopeValue == "all-cooked")
		{
			return "All Cooked Outputs";
		}
		if (scopeValue == "build-tree")
		{
			return "Build Outputs";
		}
		if (scopeValue == "shader-cache")
		{
			return "Shader Cache";
		}
		if (scopeValue == "deps")
		{
			return "Source Dependency Cache";
		}
		if (scopeValue == "logs")
		{
			return "Log Files";
		}
		if (scopeValue == "pristine")
		{
			return "Generated Workspace";
		}
		return scopeValue;
	}

	bool SupportsActionSpecificClean(const QString& operationId)
	{
		return operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build") ||
		    operationId == "cook.tools.prepare" || operationId == "cook.project" || operationId == "cook.shaders" || operationId == "cook.textures" ||
		    operationId == "cook.assets";
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
		std::filesystem::path binaryDirectory = GetDeveloperArtifactDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		std::filesystem::path libraryDirectory = GetDeveloperLibraryDirectory(repositoryRoot, "runtime-support/" + targetName.toStdString(), profileName.toStdString());
		std::filesystem::path symbolDirectory = GetSymbolDirectory(repositoryRoot) / "runtime-support" / targetName.toStdString() / profileName.toStdString();
		if (targetName == "SparkleLauncher" || targetName == "SparkleLauncherProbe")
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
		AddExplicitCleanTarget(targets, targetName + " compile database", symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"), detail);
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
		const std::filesystem::path binaryDirectory = GetProjectTargetArtifactDirectory(repositoryRoot, projectName.toStdString(), productRole.toStdString(), profileName.toStdString());
		const std::filesystem::path libraryDirectory =
		    GetDeveloperLibraryDirectory(repositoryRoot, "projects/" + projectName.toStdString() + "/" + productRole.toStdString(), profileName.toStdString());
		const std::filesystem::path symbolDirectory =
		    GetSymbolDirectory(repositoryRoot) / "projects" / projectName.toStdString() / productRole.toStdString() / profileName.toStdString();
		const std::filesystem::path executablePath = binaryDirectory / (targetName.toStdString() + ".exe");
		AddExplicitCleanTarget(targets, targetName + " executable", executablePath, detail);
		AddExplicitCleanTarget(targets, targetName + " program database", symbolDirectory / (targetName.toStdString() + ".pdb"), detail);
		AddExplicitCleanTarget(targets, targetName + " import library", libraryDirectory / (targetName.toStdString() + ".lib"), detail);
		AddExplicitCleanTarget(targets, targetName + " compile database", symbolDirectory / "obj" / (targetName.toStdString() + ".pdb"), detail);
	}

	QVector<LauncherCleanTarget> BuildActionSpecificCleanTargets(const ActionCleanTargetContext& context)
	{
		QVector<LauncherCleanTarget> targets;
		if ((context.OperationId.startsWith("project.build") || context.OperationId.startsWith("cook.")) && context.ProjectId.isEmpty())
		{
			return targets;
		}

		const auto addNamedTargets = [&context, &targets](const QString& profileName, const QStringList& targetNames, const QString& detail) {
			for (const QString& targetName : targetNames)
			{
				if (!targetName.isEmpty())
				{
					AddTargetArtifactOutputs(targets, context.RepositoryRoot, profileName, targetName, detail);
				}
			}
		};
		const auto addProjectArtifacts = [&context, &addNamedTargets, &targets](const QString& profileName, const QString& projectName, const QString& detail) {
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

		if (context.OperationId == "launcher.build.self" || context.OperationId == "workspace.build-all")
		{
			AddTargetArtifactOutputs(
			    targets,
			    context.RepositoryRoot,
			    context.EditorProfile,
			    "SparkleLauncher",
			    "Launcher direct build outputs. The currently running launcher executable is preserved until restart.",
			    context.RunningLauncherPath);
			AddTargetArtifactOutputs(targets, context.RepositoryRoot, context.EditorProfile, "SparkleLauncherProbe", "Launcher probe binary and matching direct build outputs.");
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore library",
			    GetSymbolDirectory(context.RepositoryRoot) / "launcher" / context.EditorProfile.toStdString() / "lib" / "SparkleLauncherCore.lib",
			    "Launcher support library built for the selected editor profile.");
			AddExplicitCleanTarget(
			    targets,
			    "SparkleLauncherCore program database",
			    GetSymbolDirectory(context.RepositoryRoot) / "launcher" / context.EditorProfile.toStdString() / "lib" / "SparkleLauncherCore.pdb",
			    "Launcher support library debug symbols built for the selected editor profile.");
		}

		if (context.OperationId == "project.build.editor" || context.OperationId == "workspace.build-all")
		{
			if (context.OperationId == "project.build.editor" && !context.SelectedTargets.trimmed().isEmpty())
			{
				addNamedTargets(context.EditorProfile, context.SelectedTargets.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts), "Selected editor build target outputs.");
			}
			else
			{
				addProjectArtifacts(context.EditorProfile, context.ProjectId, "Selected project editor target outputs.");
			}
		}

		if (context.OperationId == "project.build.runtime" || context.OperationId == "workspace.build-all")
		{
			if (context.OperationId == "project.build.runtime" && !context.SelectedTargets.trimmed().isEmpty())
			{
				addNamedTargets(context.RuntimeProfile, context.SelectedTargets.split(QRegularExpression("[,;\\n]"), Qt::SkipEmptyParts), "Selected runtime build target outputs.");
			}
			else
			{
				addProjectArtifacts(context.RuntimeProfile, context.ProjectId, "Selected project runtime target outputs.");
			}
		}

		if (context.OperationId == "cook.tools.prepare" || context.OperationId == "workspace.build-all")
		{
#if SPARKLE_ENABLE_CONTENT_PIPELINE
			AddTargetArtifactOutputs(targets, context.RepositoryRoot, context.EditorProfile, "AssetCooker", "AssetCooker executable outputs.");
			AddTargetArtifactOutputs(targets, context.RepositoryRoot, context.EditorProfile, "TextureCooker", "TextureCooker executable outputs.");
#endif
#if SPARKLE_ENABLE_SHADER_COMPILER
			AddTargetArtifactOutputs(targets, context.RepositoryRoot, context.EditorProfile, "ShaderCompiler", "ShaderCompiler executable outputs.");
#endif
		}

		if (context.OperationId == "cook.project")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked project content",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ProjectId.toStdString()),
			    "All cooked content for the selected project.");
			AddExplicitCleanTarget(
			    targets,
			    "Shader cache",
			    GetBuildDirectory(context.RepositoryRoot) / "Cache" / "Shaders",
			    "Shared local shader cache refreshed by cook operations.");
		}
		else if (context.OperationId == "cook.shaders")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Shader cache",
			    GetBuildDirectory(context.RepositoryRoot) / "Cache" / "Shaders",
			    "Shared local shader cache refreshed by shader cooking.");
		}
		else if (context.OperationId == "cook.textures" || context.OperationId == "cook.assets")
		{
			AddExplicitCleanTarget(
			    targets,
			    "Cooked project content",
			    GetCookedProjectDirectory(context.RepositoryRoot, context.ProjectId.toStdString()),
			    context.OperationId == "cook.textures" ? "Selected project cooked texture outputs." : "Selected project cooked mesh and material outputs.");
		}

		return targets;
	}
}
