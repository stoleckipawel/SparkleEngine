#include "LauncherOperationRequestFactory.h"

#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"

namespace SparkleLauncher
{
	WorkspaceIde ResolveSelectedWorkspaceIde(const LauncherSettings& settings)
	{
		WorkspaceIde ide = WorkspaceIde::VisualStudio;
		TryParseWorkspaceIde(settings.WorkspaceIde().toStdString(), ide);
		return ide;
	}

	QString ResolveSelectedWorkspaceIdeName(const LauncherSettings& settings)
	{
		return QString::fromStdString(DisplayName(ResolveSelectedWorkspaceIde(settings)));
	}

	WorkspaceCompiler ResolveSelectedWorkspaceCompiler(const LauncherSettings& settings)
	{
		WorkspaceCompiler compiler = WorkspaceCompiler::Msvc;
		TryParseWorkspaceCompiler(settings.WorkspaceCompiler().toStdString(), compiler);
		return compiler;
	}

	QString ResolveShaderTargetSelection(const LauncherSettings& settings)
	{
		const QString preset = settings.ShaderTargetPreset();
		if (preset == "d3d12")
		{
			return "DxilSm66";
		}
		if (preset == "vulkan")
		{
			return "SpirV16";
		}
		if (preset == "dxil-all")
		{
			return "DxilSm60, DxilSm61, DxilSm62, DxilSm63, DxilSm64, DxilSm65, DxilSm66, DxilSm67";
		}
		if (preset == "spirv-all")
		{
			return "SpirV14, SpirV15, SpirV16";
		}
		if (preset == "custom")
		{
			return settings.ShaderCustomTargets();
		}
		return "DxilSm66, SpirV16";
	}

	BuildWorkspaceOperationRequest BuildWorkspacePlanRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings)
	{
		BuildWorkspaceOperationRequest request;
		request.RepositoryRoot = repositoryRoot;
		request.ContentId = contentModel.ContentId().toStdString();
		request.EditorProfile = settings.EditorProfile().toStdString();
		request.RuntimeProfile = settings.RuntimeProfile().toStdString();
		request.PreferredIde = ResolveSelectedWorkspaceIde(settings);
		request.Compiler = ResolveSelectedWorkspaceCompiler(settings);
		request.ForceConfigure = settings.ForceConfigure();
		return request;
	}

	LauncherOperationRequest BuildQuickStartOperationRequest(
	    const LauncherOperationRequest& goalRequest,
	    const QString& operationId,
	    const QStringList& requestedLevelIds)
	{
		LauncherOperationRequest request = goalRequest;
		request.RunId.clear();
		request.OperationId = operationId;
		request.SelectedTargets.clear();
		if (!requestedLevelIds.isEmpty())
		{
			request.RequestedLevelIds = requestedLevelIds.join(',');
		}
		request.SourceDependencyId.clear();
		request.HostToolId.clear();
		request.ForceConfigure = false;
		request.ForceRecook = false;
		request.ConfirmForceRecook = false;
		request.ConfirmClean = false;
		return request;
	}

	LauncherOperationRequest BuildLauncherOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const QString& operationId)
	{
		LauncherOperationRequest request;
		request.RepositoryRoot = repositoryRoot;
		request.OperationId = operationId;
		request.ContentId = contentModel.ContentId();
		request.EditorProfile = settings.EditorProfile();
		request.RuntimeProfile = settings.RuntimeProfile();
		request.WorkspaceIde = settings.WorkspaceIde();
		request.WorkspaceCompiler = settings.WorkspaceCompiler();
		request.SelectedTargets = settings.SelectedTargets();
		request.ShaderPackages = settings.ShaderPackages();
		request.ShaderTargets = ResolveShaderTargetSelection(settings);
		request.ShaderBackend = settings.ShaderBackend();
		request.ShaderCacheDirectory = settings.ShaderCacheDirectory();
		request.GraphicsApi = settings.GraphicsApi();
		request.ShaderUseCache = settings.ShaderUseCache();
		request.ShaderEnableDebugInfo = settings.ShaderEnableDebugInfo();
		request.ShaderEnableOptimizations = settings.ShaderEnableOptimizations();
		request.ShaderWarningsAsErrors = settings.ShaderWarningsAsErrors();
		request.ShaderStripDebugInfo = settings.ShaderStripDebugInfo();
		request.CleanScope = settings.CleanScope();
		request.ForceConfigure = settings.ForceConfigure();
		request.ForceRecook = settings.ForceRecook();
		request.ConfirmForceRecook = settings.ConfirmForceRecook();
		request.ConfirmClean = settings.ConfirmClean();

		return request;
	}

	ActionCleanTargetContext BuildActionCleanTargetContext(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId)
	{
		return ActionCleanTargetContext{
		    repositoryRoot,
		    runningLauncherPath,
		    operationId,
		    contentModel.ContentId(),
		    settings.EditorProfile(),
		    settings.RuntimeProfile(),
		    settings.SelectedTargets()};
	}

	LauncherOperationRequest BuildActionCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const std::filesystem::path& runningLauncherPath,
	    const QString& operationId)
	{
		LauncherOperationRequest request = BuildLauncherOperationRequest(repositoryRoot, contentModel, settings, "workspace.clean");
		request.CleanTargets = BuildActionSpecificCleanTargets(
		    BuildActionCleanTargetContext(repositoryRoot, contentModel, settings, runningLauncherPath, operationId));
		request.ConfirmClean = false;
		return request;
	}

	LauncherOperationRequest BuildScopedCleanOperationRequest(
	    const std::filesystem::path& repositoryRoot,
	    const LauncherContentModel& contentModel,
	    const LauncherSettings& settings,
	    const QString& cleanScope,
	    const std::filesystem::path& runningLauncherPath)
	{
		LauncherOperationRequest request = BuildLauncherOperationRequest(repositoryRoot, contentModel, settings, "workspace.clean");
		request.CleanScope = cleanScope;
		request.CleanTargets.clear();
		request.PreservedPaths.clear();
		if (cleanScope == "clean-all" && !runningLauncherPath.empty())
		{
			std::error_code errorCode;
			const std::filesystem::path absoluteRunningPath = std::filesystem::absolute(runningLauncherPath, errorCode);
			const std::filesystem::path candidatePath = errorCode ? runningLauncherPath : absoluteRunningPath;
			const std::filesystem::path launcherDirectory = candidatePath.parent_path();
			if (!launcherDirectory.empty())
			{
				request.PreservedPaths.push_back(QString::fromStdString(launcherDirectory.string()));
			}
		}
		request.ConfirmClean = false;
		return request;
	}

}
