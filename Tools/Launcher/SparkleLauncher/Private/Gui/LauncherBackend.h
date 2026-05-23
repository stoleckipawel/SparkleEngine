#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

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

	class LauncherBackend final : public QObject
	{
		Q_OBJECT

	public:
		explicit LauncherBackend(QObject* parent = nullptr);

		const QVector<LauncherOperationDescriptor>& Operations() const;

	public slots:
		void RequestOperationPreview(const QString& operationId);

	signals:
		void OperationPreviewReady(const QString& operationId, const QString& title, const QString& previewText);
		void OperationPreviewFailed(const QString& operationId, const QString& message);

	private:
		void PopulateOperationCatalog();
		const LauncherOperationDescriptor* FindOperation(const QString& operationId) const;

		QVector<LauncherOperationDescriptor> m_operations;
	};
}