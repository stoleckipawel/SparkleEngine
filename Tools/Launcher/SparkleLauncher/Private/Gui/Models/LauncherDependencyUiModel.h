#pragma once

#include <QtCore/QString>

#include <filesystem>
#include <vector>

namespace SparkleLauncher
{
	struct ThirdPartyDependencyUiEntry
	{
		QString Id;
		QString Label;
		QString Version;
		QString Purpose;
		QString CacheDirectoryName;
	};

	struct DependencyGroupUiEntry
	{
		QString Id;
		QString Label;
		QString Summary;
		QString UnlockSummary;
		QString ConfigureOption;
		QString EnablementDetail;
		bool Required = false;
		bool Enabled = false;
		std::vector<ThirdPartyDependencyUiEntry> Dependencies;
	};

	struct ThirdPartyDependencyUiStatus
	{
		QString Text;
		QString Detail;
		QString State;
		bool Synced = false;
	};

	const std::vector<DependencyGroupUiEntry>& GetDependencyGroups();
	const std::vector<ThirdPartyDependencyUiEntry>& GetTrackedThirdPartyDependencies();
	ThirdPartyDependencyUiStatus BuildThirdPartyDependencyStatus(
	    const ThirdPartyDependencyUiEntry& dependency,
	    const DependencyGroupUiEntry& group,
	    const std::filesystem::path& dependencyCachePath);

	QString FormatTrackedDependencySummary(const std::filesystem::path& dependencyCachePath);
	int CountReadyDependencies(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath);
	QString DependencyGroupStatusText(const DependencyGroupUiEntry& group, int readyCount);
	QString DependencyGroupStatusState(const DependencyGroupUiEntry& group, int readyCount);
	QString FormatDependencyGroupDetail(
	    const DependencyGroupUiEntry& group,
	    const std::filesystem::path& dependencyCachePath,
	    int readyCount);
	bool OperationUsesDependencyGroup(const QString& operationId, const DependencyGroupUiEntry& group);
}
