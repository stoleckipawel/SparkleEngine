#pragma once

#include "SparkleLauncher/ProcessRunner.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <filesystem>
#include <functional>
#include <memory>

namespace SparkleLauncher
{
	enum class LauncherOperationCategory
	{
		Workspace,
		Cooking,
		Maintenance,
		Launch
	};

	struct LauncherOperationDescriptor
	{
		QString Id;
		QString Group;
		QString DisplayName;
		QString Description;
		LauncherOperationCategory Category = LauncherOperationCategory::Workspace;
	};

	struct LauncherCleanTarget
	{
		QString DisplayName;
		QString Path;
		QString Detail;
	};

	struct LauncherOperationRequest
	{
		std::filesystem::path RepositoryRoot;
		QString RunId;
		QString OperationId;
		QString ProjectId;
		QString EditorProfile;
		QString RuntimeProfile;
		QString WorkspaceIde;
		QString SelectedTargets;
		QString ShaderPackages;
		QString ShaderTargets;
		QString ShaderBackend;
		QString ShaderCacheDirectory;
		QString ShaderDebugArtifactDirectory;
		QString LaunchBackend;
		QString LaunchTarget;
		QString LaunchVSync;
		QString LaunchHighPerformanceAdapter;
		QString LaunchMeshAutoBatching;
		QString LaunchCommandLineArguments;
		QString LaunchCVars;
		QString SmokeBackend;
		QString SmokeFrameLimit;
		QString FormatMode = "check";
		QString CleanScope = "selected-cooked";
		QVector<LauncherCleanTarget> CleanTargets;
		bool LaunchSmokeTest = false;
		bool ShaderUseCache = true;
		bool ShaderEnableDebugInfo = false;
		bool ShaderEnableOptimizations = true;
		bool ShaderWarningsAsErrors = true;
		bool ShaderStripReflection = true;
		bool ShaderStripDebugInfo = true;
		bool ShaderWriteDebugArtifacts = false;
		bool ShaderWriteCookedShaderStats = false;
		bool ForceConfigure = false;
		bool ForceRecook = false;
		bool ConfirmForceRecook = false;
		bool ConfirmClean = false;
		bool SmokeTrace = false;
		bool SmokeSkipLevelSwitching = false;
	};

	class LauncherBackend final : public QObject
	{
		Q_OBJECT

	public:
		using ProcessRunnerFactory = std::function<std::unique_ptr<IProcessRunner>()>;

		explicit LauncherBackend(QObject* parent = nullptr);
		LauncherBackend(ProcessRunnerFactory processRunnerFactory, QObject* parent = nullptr);

		const QVector<LauncherOperationDescriptor>& Operations() const;

		void RequestOperationPreview(const LauncherOperationRequest& request);
		void RunOperation(LauncherOperationRequest request);

	signals:
		void OperationPreviewReady(const QString& operationId, const QString& title, const QString& previewText, bool canRun);
		void OperationPreviewFailed(const QString& operationId, const QString& message);
		void OperationStarted(const QString& runId, const QString& operationId, const QString& title);
		void OperationOutputReceived(const QString& runId, const QString& operationId, const QString& outputText);
		void OperationFinished(const QString& runId, const QString& operationId, const QString& title, const QString& statusText, int exitCode);

	private:
		void PopulateOperationCatalog();
		const LauncherOperationDescriptor* FindOperation(const QString& operationId) const;

		QVector<LauncherOperationDescriptor> m_operations;
		ProcessRunnerFactory m_processRunnerFactory;
	};
}
