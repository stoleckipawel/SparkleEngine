#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPageUtilities.h"
#include "LauncherProjectModel.h"
#include "LauncherRecoveryUiModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QStringList>
#include <QtGui/QGuiApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	namespace
	{
		QString FormatBundleDetail(const DependencyGroupUiEntry& group, int readyCount)
		{
			if (!group.Enabled)
			{
				return QStringLiteral("Optional. Off in this workspace.")
				    .arg(group.ConfigureOption);
			}

			QString detail = group.Required ? QStringLiteral("Required.") : QStringLiteral("Optional and enabled.");
			detail += QStringLiteral(" %1 of %2 packages cached.")
			              .arg(readyCount)
			              .arg(group.Dependencies.size());
			return detail;
		}

		QString FormatInventoryGroupDetail(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
		{
			if (!group.Enabled)
			{
				return QStringLiteral("Skipped because %1 is off in this workspace configuration.")
				    .arg(group.ConfigureOption);
			}

			return FormatDependencyGroupDetail(group, dependencyCachePath, CountReadyDependencies(group, dependencyCachePath));
		}
	}

	void LauncherMainWindow::AddSyncDependencyBundles(QVBoxLayout& layout, bool includeDependencyDetails)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		QVBoxLayout* bundlesLayout = AddOptionGroup(
		    layout,
		    "Repository dependency groups",
		    QString());
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			const int readyCount = CountReadyDependencies(group, dependencyCachePath);
			AddStatusRow(
			    *bundlesLayout,
			    group.Label,
			    DependencyGroupStatusText(group, readyCount),
			    FormatBundleDetail(group, readyCount),
			    DependencyGroupStatusState(group, readyCount),
			    group.Enabled ? CreateActionDependencyActions("workspace.sync-source-tiers", "Prepare Workspace", "deps", "Clean Source Dependency Cache") :
			                    CreateDisabledSourceTierActions(group));
		}

		if (!includeDependencyDetails)
		{
			return;
		}

		QVBoxLayout* inventoryLayout = AddDetailsGroup(
		    layout,
		    "Advanced dependency inventory",
		    "Raw cache status for individual third-party packages. Open this only when a specific package needs inspection or repair.",
		    false);
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			inventoryLayout->addWidget(CreateSectionLabel(group.Label));
			AddNoOptionsMessage(*inventoryLayout, FormatInventoryGroupDetail(group, dependencyCachePath));
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				const std::filesystem::path dependencyPath = dependencyCachePath / dependency.CacheDirectoryName.toStdString();
				const SourceDependencyEntry* sourceDependency = FindSourceDependency(dependency.Id.toStdString());
				const SourceDependencyValidation validation = sourceDependency != nullptr ?
				                                                 ValidateSourceDependency(*sourceDependency, dependencyCachePath) :
				                                                 SourceDependencyValidation{dependencyPath, false, {dependency.CacheDirectoryName.toStdString()}};
				std::error_code errorCode;
				const bool cachePathExists = std::filesystem::exists(dependencyPath, errorCode) && !errorCode;
				const bool dependencyReady = validation.Ready;
				AddStatusRow(
				    *inventoryLayout,
				    QStringLiteral("%1 (%2)").arg(dependency.Label, dependency.Version),
				    !group.Enabled ? "Disabled" : dependencyReady ? "Cached" : cachePathExists ? "Incomplete" : "Pending sync",
				    FormatDependencyEntryDetail(group, dependency, dependencyPath),
				    !group.Enabled ? "neutral" : dependencyReady ? "ok" : cachePathExists ? "bad" : "warning",
				    group.Enabled ? CreateTrackedDependencyActions(dependency) : CreateDisabledSourceTierActions(group));
			}
		}
	}
}
