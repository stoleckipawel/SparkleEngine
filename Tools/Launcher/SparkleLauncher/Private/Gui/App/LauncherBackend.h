#pragma once

#include "SparkleLauncher/ProcessRunner.h"
#include "SparkleLauncher/OperationModel.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <filesystem>
#include <functional>
#include <memory>

namespace SparkleLauncher
{
	class LauncherOperationService;
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
		QString ContentId;
		QString EditorProfile;
		QString RuntimeProfile;
		QString WorkspaceIde;
		QString SelectedTargets;
		QString RequestedLevelIds;
		QString SourceDependencyGroupId;
		QString SourceDependencyConfigureOption;
		QString ShaderPackages;
		QString ShaderTargets;
		QString ShaderBackend;
		QString ShaderCacheDirectory;
		QString LaunchBackend;
		QString LaunchTarget;
		QString LaunchStartupLevel;
		QString LaunchVSync;
		QString LaunchHighPerformanceAdapter;
		QString LaunchCommandLineArguments;
		QString LaunchCVars;
		QString CleanScope = "cooked";
		QVector<LauncherCleanTarget> CleanTargets;
		QVector<QString> PreservedPaths;
		bool ShaderUseCache = true;
		bool ShaderEnableDebugInfo = false;
		bool ShaderEnableOptimizations = true;
		bool ShaderWarningsAsErrors = true;
		bool ShaderStripDebugInfo = true;
		bool ForceConfigure = false;
		bool ForceRecook = false;
		bool ConfirmForceRecook = false;
		bool ConfirmClean = false;
	};

	class LauncherBackend final : public QObject
	{
		Q_OBJECT

	public:
		using ProcessRunnerFactory = std::function<std::unique_ptr<IProcessRunner>()>;

		explicit LauncherBackend(QObject* parent = nullptr);
		LauncherBackend(ProcessRunnerFactory processRunnerFactory, QObject* parent = nullptr);
		~LauncherBackend() override;

		const QVector<LauncherOperationDescriptor>& Operations() const;

		void RequestOperationPreview(const LauncherOperationRequest& request);
		void RunOperation(LauncherOperationRequest request);
		bool CancelOperation(const QString& runId);

	signals:
		void OperationPreviewReady(const QString& operationId, const QString& title, const QString& previewText, bool canRun);
		void OperationPreviewFailed(const QString& operationId, const QString& message);
		void OperationStarted(const QString& runId, const QString& operationId, const QString& title);
		void OperationOutputReceived(const QString& runId, const QString& operationId, const QString& outputText);
		void OperationFinished(
		    const QString& runId,
		    const QString& operationId,
		    const QString& title,
		    const QString& statusText,
		    int exitCode);

	private:
		void PopulateOperationCatalog();
		void QueueOperationOutput(QString runId, QString operationId, QString outputText);
		void QueueOperationFinished(QString runId, QString operationId, QString title, OperationRecord record);
		const LauncherOperationDescriptor* FindOperation(const QString& operationId) const;

		QVector<LauncherOperationDescriptor> m_operations;
		std::unique_ptr<LauncherOperationService> m_operationService;
	};
}
