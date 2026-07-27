#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVector>

#include <filesystem>

namespace SparkleLauncher
{
	struct LauncherProjectSummary
	{
		QString Id;
		QString DisplayName;
		std::filesystem::path RootPath;
	};

	class LauncherProjectModel final : public QObject
	{
		Q_OBJECT

	public:
		explicit LauncherProjectModel(QObject* parent = nullptr);

		const QVector<LauncherProjectSummary>& Projects() const;
		const QString& ActiveProjectId() const;
		const LauncherProjectSummary* ActiveProject() const;

	public slots:
		void Refresh(const std::filesystem::path& repositoryRoot);

	signals:
		void ProjectsChanged();
		void ProjectDiscoveryFailed(const QString& message);

	private:
		static QString ChooseInitialProjectId(const QVector<LauncherProjectSummary>& projects);

		QVector<LauncherProjectSummary> m_projects;
		QString m_activeProjectId;
	};
}
