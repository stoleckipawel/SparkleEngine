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

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QStandardItemModel>
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
#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{

		struct LauncherStartupLevelOption
		{
			QString DisplayName;
			QString Id;
			bool Synced = false;
			bool Ready = false;
			bool StartupDefault = false;
		};

		bool SetLevelDefaultIncluded(const std::filesystem::path& projectRoot, const QString& levelId, bool enabled)
		{
			std::string errorMessage;
			return ProjectLevelCatalogFile::SetLevelDefaultIncluded(
			    projectRoot,
			    levelId.toStdString(),
			    enabled,
			    errorMessage);
		}

		bool SetOptionalPackAvailable(const std::filesystem::path& projectRoot, const QString& packId, bool enabled)
		{
			std::string errorMessage;
			return ProjectLevelCatalogFile::SetOptionalContentPackAvailable(
			    projectRoot,
			    packId.toStdString(),
			    enabled,
			    errorMessage);
		}

		bool CatalogPathExists(const std::filesystem::path& path)
		{
			std::error_code errorCode;
			return std::filesystem::exists(path, errorCode) && !errorCode;
		}

		QString DisplayNameOrId(std::string_view displayName, std::string_view id)
		{
			return QString::fromStdString(
			    std::string(displayName.empty() ? id : displayName));
		}

		const LauncherProjectSummary* FindActiveProject(const LauncherProjectModel& projectModel)
		{
			const QString activeProjectId = projectModel.ActiveProjectId();
			for (const LauncherProjectSummary& project : projectModel.Projects())
			{
				if (project.Id == activeProjectId)
				{
					return &project;
				}
			}
			return nullptr;
		}

		bool LevelSynced(const ProjectLevelCatalogEntry& level)
		{
			return level.required || level.defaultIncluded;
		}

		QVector<LauncherStartupLevelOption> BuildStartupLevelEntries(const LauncherProjectSummary* activeProject)
		{
			QVector<LauncherStartupLevelOption> options;
			if (activeProject == nullptr)
			{
				return options;
			}

			ProjectLevelCatalog catalog;
			std::string errorMessage;
			if (!ProjectLevelCatalogFile::Load(
			        activeProject->RootPath,
			        catalog,
			        errorMessage))
			{
				return options;
			}

			const auto addLevel = [&catalog, &options, activeProject](const ProjectLevelCatalogEntry& level) {
				options.push_back(LauncherStartupLevelOption{
				    .DisplayName = DisplayNameOrId(level.displayName, level.id),
				    .Id = QString::fromStdString(level.id),
				    .Synced = LevelSynced(level),
				    .Ready = catalog.IsLevelReady(activeProject->RootPath, level),
				    .StartupDefault = level.startupDefault});
			};
			for (const ProjectLevelCatalogEntry& level : catalog.levels)
			{
				if (level.startupDefault)
				{
					addLevel(level);
				}
			}
			for (const ProjectLevelCatalogEntry& level : catalog.levels)
			{
				if (!level.startupDefault)
				{
					addLevel(level);
				}
			}
			return options;
		}

		QString RelativeProjectPath(const std::filesystem::path& projectRoot, const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return QString();
			}

			std::error_code errorCode;
			const std::filesystem::path relative = std::filesystem::relative(path, projectRoot, errorCode);
			return QString::fromStdString((errorCode ? path : relative).generic_string());
		}

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



	void LauncherMainWindow::AddSyncDependencyBundles(QVBoxLayout& layout)
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
	}

	void LauncherMainWindow::AddSyncLevelContentGroups(QVBoxLayout& layout)
	{
		QVBoxLayout* levelLayout = AddOptionGroup(
		    layout,
		    "Project level sync groups",
		    "Selectable project levels and the content roots they require.");
		const LauncherProjectSummary* activeProject = FindActiveProject(m_projectModel);
		if (activeProject == nullptr)
		{
			AddNoOptionsMessage(*levelLayout, "No active project was discovered.");
			return;
		}

		ProjectLevelCatalog catalog;
		std::string catalogError;
		if (!ProjectLevelCatalogFile::Load(
		        activeProject->RootPath,
		        catalog,
		        catalogError))
		{
			AddNoOptionsMessage(*levelLayout, "The active project has no Levels.catalog.");
			return;
		}

		for (const ProjectLevelCatalogEntry& level : catalog.levels)
		{
			const bool ready =
			    catalog.IsLevelReady(activeProject->RootPath, level);
			const bool synced = LevelSynced(level);
			QStringList traits;
			if (level.required)
			{
				traits.push_back("required");
			}
			if (synced)
			{
				traits.push_back("synced");
			}
			if (level.startupDefault)
			{
				traits.push_back("startup default");
			}
			if (!level.optionalContentPackId.empty())
			{
				traits.push_back(
				    QStringLiteral("pack %1")
				        .arg(QString::fromStdString(level.optionalContentPackId)));
			}
			const QString detail = QStringLiteral("%1%2")
			                           .arg(RelativeProjectPath(activeProject->RootPath, level.sourcePath))
			                           .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", ")));
			QCheckBox* syncBox =
			    new QCheckBox(
			        level.required
			            ? QStringLiteral("Required")
			            : QStringLiteral("Sync"),
			        this);
			syncBox->setChecked(synced);
			syncBox->setEnabled(!level.required);
			RegisterFocusable(syncBox);
			connect(syncBox, &QCheckBox::toggled, this, [this, projectRoot = activeProject->RootPath, levelId = QString::fromStdString(level.id), syncBox](bool checked) {
				if (!SetLevelDefaultIncluded(projectRoot, levelId, checked))
				{
					const QSignalBlocker blocker(syncBox);
					syncBox->setChecked(!checked);
					return;
				}
				ScheduleUiRefresh(false);
			});
			AddStatusRow(
			    *levelLayout,
			    DisplayNameOrId(level.displayName, level.id),
			    !synced ? "Off" : ready ? (level.required ? "Required" : "Synced") : "Missing",
			    detail,
			    !synced ? "neutral" : ready ? "ok" : (level.required ? "bad" : "warning"),
			    syncBox);
		}

		if (catalog.optionalContentPacks.empty())
		{
			return;
		}

		QVBoxLayout* packLayout = AddDetailsGroup(
		    layout,
		    "Optional content packs",
		    "External or optional content roots referenced by level sync groups.",
		    true);
		for (const auto& [packId, pack] : catalog.optionalContentPacks)
		{
			const bool rootReady =
			    pack.rootPath.empty() ||
			    CatalogPathExists(pack.rootPath);
			const bool ready = pack.available && rootReady;
			QStringList traits;
			if (pack.external)
			{
				traits.push_back("external");
			}
			if (!pack.available)
			{
				traits.push_back("not synced");
			}
			QCheckBox* syncBox = new QCheckBox(QStringLiteral("Sync"), this);
			syncBox->setChecked(pack.available);
			RegisterFocusable(syncBox);
			connect(syncBox, &QCheckBox::toggled, this, [this, projectRoot = activeProject->RootPath, capturedPackId = QString::fromStdString(packId), syncBox](bool checked) {
				if (!SetOptionalPackAvailable(projectRoot, capturedPackId, checked))
				{
					const QSignalBlocker blocker(syncBox);
					syncBox->setChecked(!checked);
					return;
				}
				ScheduleUiRefresh(false);
			});
			AddStatusRow(
			    *packLayout,
			    DisplayNameOrId(pack.displayName, pack.id),
			    !pack.available ? "Off" : ready ? "Present" : (pack.external ? "External" : "Missing"),
			    QStringLiteral("%1%2")
			        .arg(RelativeProjectPath(activeProject->RootPath, pack.rootPath))
			        .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", "))),
			    !pack.available ? "neutral" : ready ? "ok" : "warning",
			    syncBox);
		}
	}

	QComboBox* LauncherMainWindow::CreateStartupLevelCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setProperty("StartupLevelSelector", true);
		combo->setToolTip("Startup level used by editor and runtime launches.");
		combo->setAccessibleName("Startup level");
		combo->setAccessibleDescription("Startup level used by editor and runtime launches.");
		RegisterFocusable(combo);
		m_startupLevelSelectors.push_back(combo);
		PopulateStartupLevelCombo(*combo);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, this]() {
			const QString levelId = combo->currentData().toString();
			if (!levelId.isEmpty())
			{
				m_settings.SetLaunchStartupLevel(levelId);
			}
		});
		return combo;
	}

	void LauncherMainWindow::PopulateStartupLevelCombo(QComboBox& combo)
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		const QVector<LauncherStartupLevelOption> options = BuildStartupLevelEntries(FindActiveProject(m_projectModel));
		if (options.empty())
		{
			combo.addItem(QStringLiteral("No catalog levels"), QString());
			combo.setEnabled(false);
			combo.setToolTip("No catalog levels are available for the active project.");
			return;
		}

		combo.setEnabled(true);
		combo.setToolTip("Startup level used by editor and runtime launches.");
		const QIcon syncedIcon = m_icons.Icon(LauncherIcon::Done, QColor(LauncherUi::Color::StateSuccess));
		const QIcon missingIcon = m_icons.Icon(LauncherIcon::Failed, QColor(LauncherUi::Color::StateWarning));
		const QIcon unsyncedIcon = m_icons.Icon(LauncherIcon::Sync, QColor(LauncherUi::Color::StateQueued));
		int selectedIndex = -1;
		int startupDefaultIndex = -1;
		int firstSelectableIndex = -1;
		for (const LauncherStartupLevelOption& option : options)
		{
			const bool selectable = option.Synced && option.Ready;
			const QString status = !option.Synced ? "not synced" : option.Ready ? "synced" : "missing";
			const QIcon icon = selectable ? syncedIcon : option.Synced ? missingIcon : unsyncedIcon;
			combo.addItem(icon, option.DisplayName, option.Id);
			const int row = combo.count() - 1;
			combo.setItemData(row, QStringLiteral("%1: %2").arg(option.DisplayName, status), Qt::ToolTipRole);
			if (QStandardItemModel* model = qobject_cast<QStandardItemModel*>(combo.model()))
			{
				if (QStandardItem* item = model->item(row))
				{
					item->setEnabled(selectable);
				}
			}
			if (selectable && firstSelectableIndex < 0)
			{
				firstSelectableIndex = row;
			}
			if (selectable && option.StartupDefault)
			{
				startupDefaultIndex = row;
			}
			if (selectable && option.Id == m_settings.LaunchStartupLevel())
			{
				selectedIndex = row;
			}
		}

		if (selectedIndex < 0)
		{
			selectedIndex = startupDefaultIndex >= 0 ? startupDefaultIndex : firstSelectableIndex;
		}
		const bool hasSelectableLevel = selectedIndex >= 0;
		combo.setEnabled(hasSelectableLevel);
		if (!hasSelectableLevel)
		{
			combo.setToolTip("No synced startup level is ready for editor or runtime launch.");
		}
		combo.setCurrentIndex(hasSelectableLevel ? selectedIndex : 0);
		const QString effectiveLevelId = combo.currentData().toString();
		if (hasSelectableLevel && !effectiveLevelId.isEmpty() && m_settings.LaunchStartupLevel() != effectiveLevelId)
		{
			m_settings.SetLaunchStartupLevel(effectiveLevelId);
		}
	}

	void LauncherMainWindow::PopulateStartupLevelSelectors()
	{
		for (QComboBox* combo : m_startupLevelSelectors)
		{
			if (combo != nullptr)
			{
				PopulateStartupLevelCombo(*combo);
			}
		}
	}

	QVector<QPair<QString, QString>> LauncherMainWindow::BuildStartupLevelOptions() const
	{
		QVector<QPair<QString, QString>> options;
		for (const LauncherStartupLevelOption& option : BuildStartupLevelEntries(FindActiveProject(m_projectModel)))
		{
			if (option.Synced && option.Ready)
			{
				options.push_back({option.DisplayName, option.Id});
			}
		}
		return options;
	}

	QString LauncherMainWindow::ResolveStartupLevelDisplayName() const
	{
		const QVector<QPair<QString, QString>> options = BuildStartupLevelOptions();
		const QString selectedLevel = m_settings.LaunchStartupLevel();
		for (const QPair<QString, QString>& option : options)
		{
			if (!selectedLevel.isEmpty() && option.second == selectedLevel)
			{
				return option.first;
			}
		}
		if (!selectedLevel.isEmpty())
		{
			return selectedLevel;
		}
		return options.empty() ? QStringLiteral("project default") : options.front().first;
	}
}
