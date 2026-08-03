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
#include "LauncherContentModel.h"
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
#include <QtCore/QSignalBlocker>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
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
	void LauncherMainWindow::AddSyncDependencies(QVBoxLayout& layout, bool optional)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			if (group.Required == optional)
			{
				continue;
			}

			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				const ThirdPartyDependencyUiStatus status = BuildThirdPartyDependencyStatus(dependency, group, dependencyCachePath);
				AddStatusRow(
				    layout,
				    dependency.Label,
				    status.Text,
				    status.Detail,
				    status.State,
				    group.Enabled
				        ? CreateActionDependencyActions("workspace.sync-source-tiers", "Sync Code", "deps", "Clean Source Dependency Cache")
				        : CreateDisabledSourceDependencyActions(group));
			}
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
		if (!model.Loaded)
		{
			combo.addItem(QStringLiteral("Level catalog unavailable"), QString());
			combo.setEnabled(false);
			combo.setToolTip(model.LoadError.isEmpty() ? QStringLiteral("The level catalog could not be loaded.") : model.LoadError);
			return;
		}

		const QVector<LauncherStartupLevelUiEntry>& options = model.StartupLevels;
		if (options.empty())
		{
			combo.addItem(QStringLiteral("No catalog levels"), QString());
			combo.setEnabled(false);
			combo.setToolTip("No catalog levels are available.");
			return;
		}

		combo.setEnabled(true);
		combo.setToolTip("Startup level used by editor and runtime launches.");
		ApplyStartupLevelSelection(combo, AppendStartupLevelOptions(combo, options));
	}

	int LauncherMainWindow::AppendStartupLevelOptions(QComboBox& combo, const QVector<LauncherStartupLevelUiEntry>& options)
	{
		const QIcon readyIcon = m_icons.Icon(LauncherIcon::Done, QColor(LauncherUi::Color::StateSuccess));
		const QIcon missingIcon = m_icons.Icon(LauncherIcon::Failed, QColor(LauncherUi::Color::StateWarning));
		int selectedIndex = -1;
		int firstSelectableIndex = -1;
		for (const LauncherStartupLevelUiEntry& option : options)
		{
			const bool selectable = option.Ready;
			const QIcon icon = selectable ? readyIcon : missingIcon;
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
			if (selectable && option.Id == m_settings.LaunchStartupLevel())
			{
				selectedIndex = row;
			}
		}

		if (selectedIndex < 0)
		{
			selectedIndex = firstSelectableIndex;
		}

		return selectedIndex;
	}

	void LauncherMainWindow::ApplyStartupLevelSelection(QComboBox& combo, int selectedIndex)
	{
		if (selectedIndex < 0)
		{
			combo.insertItem(0, QStringLiteral("Built-in Empty"), QString());
			combo.setCurrentIndex(0);
			combo.setEnabled(true);
			combo.setToolTip("No selected map is ready. Launches use the built-in empty scene.");
			if (!m_settings.LaunchStartupLevel().isEmpty())
			{
				m_settings.SetLaunchStartupLevel(QString());
			}
			return;
		}

		combo.setEnabled(true);
		combo.setCurrentIndex(selectedIndex);
		const QString effectiveLevelId = combo.currentData().toString();
		if (!effectiveLevelId.isEmpty() && m_settings.LaunchStartupLevel() != effectiveLevelId)
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
			if (option.Ready)
			{
				options.push_back({option.DisplayName, option.Id});
			}
		}
		return options;
	}

	LauncherLevelUiModel LauncherMainWindow::BuildLevelUiModel() const
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return {};
		}

		return LauncherLevelUiModel::Build(*content);
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
		return options.empty() ? QStringLiteral("Built-in Empty") : options.front().first;
	}
}
