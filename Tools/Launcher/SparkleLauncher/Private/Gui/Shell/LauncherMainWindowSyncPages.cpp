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
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;
	static constexpr int kSpaceMedium = LauncherUi::Space::Medium;
	static constexpr int kFieldLabelWidth = LauncherUi::Row::FieldLabelWidth;
	static constexpr int kStatusChipColumnWidth = LauncherUi::Row::StatusChipColumnWidth;
	static constexpr int kStatusActionColumnWidth = LauncherUi::Row::StatusActionColumnWidth;
	static constexpr const char* kColorStateReady = LauncherUi::Color::StateSuccess;
	static constexpr const char* kColorStateWarning = LauncherUi::Color::StateWarning;	QFrame* LauncherMainWindow::CreateSourceTierCard(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		const int readyCount = CountReadyDependencies(group, dependencyCachePath);
		const QString state = DependencyGroupStatusState(group, readyCount);
		QFrame* card = new QFrame(this);
		card->setObjectName("SourceTierCard");
		card->setProperty("State", state);
		card->setMinimumHeight(LauncherUi::SourceTier::MinHeight);
		QVBoxLayout* cardLayout = new QVBoxLayout(card);
		cardLayout->setContentsMargins(LauncherUi::SourceTier::Margins());
		cardLayout->setSpacing(LauncherUi::SourceTier::Spacing);

		QHBoxLayout* titleRow = new QHBoxLayout();
		titleRow->setContentsMargins(0, 0, 0, 0);
		titleRow->setSpacing(kSpaceSmall);
		QLabel* title = new QLabel(group.Label, card);
		title->setObjectName("SourceTierTitle");
		titleRow->addWidget(title, 1);
		QLabel* chip = new QLabel(DependencyGroupStatusText(group, readyCount), card);
		chip->setObjectName("SourceTierChip");
		chip->setProperty("State", state);
		titleRow->addWidget(chip, 0, Qt::AlignRight | Qt::AlignTop);
		cardLayout->addLayout(titleRow);

		QLabel* summary = new QLabel(group.Enabled ? group.UnlockSummary : FormatDependencyGroupDetail(group, dependencyCachePath, readyCount), card);
		summary->setObjectName("SourceTierText");
		summary->setWordWrap(true);
		cardLayout->addWidget(summary, 1);

		QHBoxLayout* metaRow = new QHBoxLayout();
		metaRow->setContentsMargins(0, 0, 0, 0);
		metaRow->setSpacing(kSpaceSmall);
		const QString metaText = group.Required ? "Required" : (group.Enabled ? "Optional enabled" : "Optional disabled");
		QLabel* meta = new QLabel(metaText, card);
		meta->setObjectName("SourceTierMeta");
		metaRow->addWidget(meta, 1);
		if (group.Enabled)
		{
			QWidget* actions = CreateActionDependencyActions("workspace.sync-source-tiers", "Sync Source Tiers", "deps", "Clean Source Dependency Cache");
			if (actions != nullptr)
			{
				actions->setParent(card);
				metaRow->addWidget(actions, 0, Qt::AlignRight);
			}
		}
		else
		{
			QWidget* actions = CreateDisabledSourceTierActions(group);
			if (actions != nullptr)
			{
				actions->setParent(card);
				metaRow->addWidget(actions, 0, Qt::AlignRight);
			}
		}
		cardLayout->addLayout(metaRow);
		return card;
	}

	void LauncherMainWindow::AddSourceTierCards(QVBoxLayout& layout, const QString& title, const QString& detail, bool includeDependencyDetails)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		QVBoxLayout* tiersLayout = AddOptionGroup(layout, title, detail);
		ResponsiveCardGridWidget* tierGrid = new ResponsiveCardGridWidget(300, 380, 4, 12, 12, this);
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			tierGrid->AddCard(CreateSourceTierCard(group, dependencyCachePath));
		}
		tiersLayout->addWidget(tierGrid);

		if (!includeDependencyDetails)
		{
			return;
		}

		QVBoxLayout* inventoryLayout = AddDetailsGroup(
		    layout,
		    "Dependency Inventory",
		    "Individual source dependencies and cache actions. Keep this closed unless a specific cache needs inspection or repair.",
		    false);
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			QVBoxLayout* dependenciesLayout = AddDetailsGroup(
			    *inventoryLayout,
			    group.Label + " Contents",
			    group.Enabled ? group.UnlockSummary : FormatDependencyGroupDetail(group, dependencyCachePath, 0),
			    false);
			for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
			{
				const std::filesystem::path dependencyPath = dependencyCachePath / dependency.CacheDirectoryName.toStdString();
				const bool dependencyReady = DirectoryHasEntries(dependencyPath);
				AddStatusRow(
				    *dependenciesLayout,
				    QStringLiteral("%1 (%2)").arg(dependency.Label, dependency.Version),
				    !group.Enabled ? "Disabled" : dependencyReady ? "Cached" : "Pending sync",
				    FormatDependencyEntryDetail(group, dependency, dependencyPath),
				    !group.Enabled ? "neutral" : dependencyReady ? "ok" : "warning",
				    group.Enabled ? CreateTrackedDependencyActions(dependency) : CreateDisabledSourceTierActions(group));
			}
		}
	}
}

