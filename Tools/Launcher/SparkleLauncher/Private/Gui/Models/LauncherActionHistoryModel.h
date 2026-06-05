#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>

#include <filesystem>

namespace SparkleLauncher
{
	struct LauncherActionHistoryRecord
	{
		QString CompletedAtUtc;
		QString ResultText;
		int ExitCode = 0;
	};

	class LauncherActionHistoryModel
	{
	public:
		void Load(const std::filesystem::path& repositoryRoot);
		void Save(const std::filesystem::path& repositoryRoot) const;
		void RecordCompletion(const QString& operationId, const QString& completedAtUtc, const QString& resultText, int exitCode);
		bool Dismiss(const QString& operationId);

		const LauncherActionHistoryRecord* Find(const QString& operationId) const;
		int FailureCount() const;

	private:
		QHash<QString, LauncherActionHistoryRecord> m_records;
	};
}
