#pragma once

#include <QtCore/QObject>
#include <QtCore/QString>

#include <filesystem>

namespace SparkleLauncher
{
	struct LauncherContentSummary
	{
		QString Id;
		QString DisplayName;
		std::filesystem::path RootPath;
	};

	class LauncherContentModel final : public QObject
	{
		Q_OBJECT

	public:
		explicit LauncherContentModel(QObject* parent = nullptr);

		const QString& ContentId() const;
		const LauncherContentSummary* Content() const;

	public slots:
		void Refresh(const std::filesystem::path& repositoryRoot);

	signals:
		void ContentChanged();
		void ContentDiscoveryFailed(const QString& message);

	private:
		LauncherContentSummary m_content;
		bool m_hasContent = false;
	};
}
