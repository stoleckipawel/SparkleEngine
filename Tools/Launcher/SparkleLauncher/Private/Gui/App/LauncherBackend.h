#pragma once

#include "LauncherOperationRequest.h"
#include "LauncherProgressStreamDecoder.h"
#include "SparkleLauncher/OperationModel.h"
#include "SparkleLauncher/ProcessRunner.h"

#include <QtCore/QHash>
#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <functional>
#include <memory>
#include <string>

namespace SparkleLauncher
{
	class LauncherOperationService;
	struct OperationRecord;

	struct LauncherOperationDescriptor
	{
		QString Id;
		QString Group;
		QString DisplayName;
		QString Description;
		LauncherOperationCategory Category = LauncherOperationCategory::Workspace;
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
		void OperationProgressReceived(
		    const QString& runId,
		    const QString& operationId,
		    const QString& phase,
		    quint64 completed,
		    quint64 total);
		void OperationFinished(
		    const QString& runId,
		    const QString& operationId,
		    const QString& title,
		    const QString& statusText,
		    int exitCode,
		    Process::ChildProcessStartFailure processStartFailure);

	private:
		void PopulateOperationCatalog();
		void QueueOperationOutput(QString runId, QString operationId, std::string outputText);
		void QueueOperationFinished(QString runId, QString operationId, QString title, const OperationRecord& record);
		const LauncherOperationDescriptor* FindOperation(const QString& operationId) const;

		QVector<LauncherOperationDescriptor> m_operations;
		QHash<QString, LauncherProgressStreamDecoder> m_progressDecoders;
		std::unique_ptr<LauncherOperationService> m_operationService;
	};
}
