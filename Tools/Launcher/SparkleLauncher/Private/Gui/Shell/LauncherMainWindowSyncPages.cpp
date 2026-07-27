#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
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

#include <filesystem>
#include <string>

namespace SparkleLauncher
{
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
			    FormatDependencyGroupDetail(
			        group,
			        dependencyCachePath,
			        readyCount),
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
		const LauncherProjectSummary* activeProject =
		    m_projectModel.ActiveProject();
		if (activeProject == nullptr)
		{
			AddNoOptionsMessage(*levelLayout, "No active project was discovered.");
			return;
		}

		const LauncherLevelUiModel model =
		    LauncherLevelUiModel::Build(*activeProject);
		if (!model.Loaded)
		{
			AddNoOptionsMessage(*levelLayout, "The active project has no Levels.catalog.");
			return;
		}

		AddSyncLevelRows(*levelLayout, *activeProject, model);
		AddSyncContentPackRows(layout, *activeProject, model);
	}

	void LauncherMainWindow::AddSyncLevelRows(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherLevelUiModel& model)
	{
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			AddSyncLevelRow(layout, project, level);
		}
	}

	void LauncherMainWindow::AddSyncLevelRow(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherLevelUiEntry& level)
	{
		QCheckBox* syncBox =
		    new QCheckBox(
		        level.Required
		            ? QStringLiteral("Required")
		            : QStringLiteral("Sync"),
		        this);
		syncBox->setChecked(level.Synced);
		syncBox->setEnabled(!level.Required);
		RegisterFocusable(syncBox);

		connect(
		    syncBox,
		    &QCheckBox::toggled,
		    this,
		    [this, projectRoot = project.RootPath, levelId = level.Id, syncBox](bool checked)
		    {
			    std::string errorMessage;
			    if (!ProjectLevelCatalogFile::SetLevelDefaultIncluded(
			            projectRoot,
			            levelId.toStdString(),
			            checked,
			            errorMessage))
			    {
				    const QSignalBlocker blocker(syncBox);
				    syncBox->setChecked(!checked);
				    return;
			    }

			    ScheduleUiRefresh(false);
		    });

		AddStatusRow(
		    layout,
		    level.DisplayName,
		    level.Status,
		    level.Detail,
		    level.State,
		    syncBox);
	}

	void LauncherMainWindow::AddSyncContentPackRows(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherLevelUiModel& model)
	{
		if (model.ContentPacks.empty())
		{
			return;
		}

		QVBoxLayout* packLayout = AddDetailsGroup(
		    layout,
		    "Optional content packs",
		    "External or optional content roots referenced by level sync groups.",
		    true);
		for (const LauncherContentPackUiEntry& pack : model.ContentPacks)
		{
			AddSyncContentPackRow(*packLayout, project, pack);
		}
	}

	void LauncherMainWindow::AddSyncContentPackRow(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherContentPackUiEntry& pack)
	{
		QCheckBox* syncBox = new QCheckBox(QStringLiteral("Sync"), this);
		syncBox->setChecked(pack.Available);
		RegisterFocusable(syncBox);

		connect(
		    syncBox,
		    &QCheckBox::toggled,
		    this,
		    [this, projectRoot = project.RootPath, packId = pack.Id, syncBox](bool checked)
		    {
			    std::string errorMessage;
			    if (!ProjectLevelCatalogFile::SetOptionalContentPackAvailable(
			            projectRoot,
			            packId.toStdString(),
			            checked,
			            errorMessage))
			    {
				    const QSignalBlocker blocker(syncBox);
				    syncBox->setChecked(!checked);
				    return;
			    }

			    ScheduleUiRefresh(false);
		    });

		AddStatusRow(
		    layout,
		    pack.DisplayName,
		    pack.Status,
		    pack.Detail,
		    pack.State,
		    syncBox);
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
		connect(
		    combo,
		    static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
		    [combo, this]()
		    {
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

		const LauncherLevelUiModel model = BuildLevelUiModel();
		const QVector<LauncherStartupLevelUiEntry>& options =
		    model.StartupLevels;
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
		for (const LauncherStartupLevelUiEntry& option : options)
		{
			const bool selectable = option.Synced && option.Ready;
			const QIcon icon = selectable ? syncedIcon : option.Synced ? missingIcon : unsyncedIcon;
			combo.addItem(icon, option.DisplayName, option.Id);
			const int row = combo.count() - 1;
			combo.setItemData(row, QStringLiteral("%1: %2").arg(option.DisplayName, option.Status), Qt::ToolTipRole);
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
		const LauncherLevelUiModel model = BuildLevelUiModel();
		for (const LauncherStartupLevelUiEntry& option : model.StartupLevels)
		{
			if (option.Synced && option.Ready)
			{
				options.push_back({option.DisplayName, option.Id});
			}
		}
		return options;
	}

	LauncherLevelUiModel LauncherMainWindow::BuildLevelUiModel() const
	{
		const LauncherProjectSummary* activeProject =
		    m_projectModel.ActiveProject();
		if (activeProject == nullptr)
		{
			return {};
		}

		return LauncherLevelUiModel::Build(*activeProject);
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
