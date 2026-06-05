#include "LauncherActionHistoryModel.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QStringList>

#include <fstream>
#include <system_error>

namespace SparkleLauncher
{
	namespace
	{
		QString SanitizeActionHistoryField(QString value)
		{
			value.replace('\t', ' ');
			value.replace('\r', ' ');
			value.replace('\n', ' ');
			return value.trimmed();
		}
	}

	void LauncherActionHistoryModel::Load(const std::filesystem::path& repositoryRoot)
	{
		m_records.clear();

		const std::filesystem::path historyPath = GetLauncherStatePaths(repositoryRoot).ActionHistoryPath;
		std::ifstream stream(historyPath);
		if (!stream.is_open())
		{
			return;
		}

		std::string line;
		while (std::getline(stream, line))
		{
			const QStringList fields = QString::fromStdString(line).split('\t');
			if (fields.size() < 4)
			{
				continue;
			}

			LauncherActionHistoryRecord record;
			record.CompletedAtUtc = fields[1].trimmed();
			record.ResultText = fields[2].trimmed();
			bool exitCodeOk = false;
			record.ExitCode = fields[3].trimmed().toInt(&exitCodeOk);
			if (!exitCodeOk)
			{
				record.ExitCode = -1;
			}
			m_records.insert(fields[0].trimmed(), record);
		}
	}

	void LauncherActionHistoryModel::Save(const std::filesystem::path& repositoryRoot) const
	{
		const LauncherStatePaths statePaths = GetLauncherStatePaths(repositoryRoot);
		std::error_code errorCode;
		std::filesystem::create_directories(statePaths.RootDirectory, errorCode);

		std::ofstream stream(statePaths.ActionHistoryPath, std::ios::out | std::ios::trunc);
		if (!stream.is_open())
		{
			return;
		}

		for (auto it = m_records.constBegin(); it != m_records.constEnd(); ++it)
		{
			stream << SanitizeActionHistoryField(it.key()).toStdString() << '\t'
			       << SanitizeActionHistoryField(it.value().CompletedAtUtc).toStdString() << '\t'
			       << SanitizeActionHistoryField(it.value().ResultText).toStdString() << '\t'
			       << it.value().ExitCode << '\n';
		}
	}

	void LauncherActionHistoryModel::RecordCompletion(const QString& operationId, const QString& completedAtUtc, const QString& resultText, int exitCode)
	{
		LauncherActionHistoryRecord record;
		record.CompletedAtUtc = completedAtUtc;
		record.ResultText = resultText;
		record.ExitCode = exitCode;
		m_records.insert(operationId, record);
	}

	bool LauncherActionHistoryModel::Dismiss(const QString& operationId)
	{
		if (!m_records.contains(operationId))
		{
			return false;
		}
		m_records.remove(operationId);
		return true;
	}

	const LauncherActionHistoryRecord* LauncherActionHistoryModel::Find(const QString& operationId) const
	{
		const auto found = m_records.constFind(operationId);
		return found == m_records.constEnd() ? nullptr : &found.value();
	}

	int LauncherActionHistoryModel::FailureCount() const
	{
		int failureCount = 0;
		for (auto it = m_records.constBegin(); it != m_records.constEnd(); ++it)
		{
			if (it.value().ExitCode != 0)
			{
				++failureCount;
			}
		}
		return failureCount;
	}
}
