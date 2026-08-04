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
#include "LauncherQuickStartPlanner.h"
#include "LauncherContentModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include <QtCore/QCoreApplication>
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
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <filesystem>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
		m_quickStartButtons.clear();
		const LauncherLevelUiModel levelModel = BuildLevelUiModel();
		const auto planQuickStart = [this, &levelModel](const QString& operationId)
		{
			LauncherOperationRequest request = BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, operationId);
			return PlanLauncherQuickStartStep(request, levelModel, {});
		};

		const LauncherCapabilityResolution editorPlan = planQuickStart("launch.editor");
		const LauncherCapabilityResolution runtimePlan = planQuickStart("launch.runtime");
		QWidget* quickStartBody = new QWidget(this);
		quickStartBody->setObjectName("QuickStartBody");
		quickStartBody->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		QVBoxLayout* bodyLayout = new QVBoxLayout(quickStartBody);
		bodyLayout->setContentsMargins(
		    LauncherUi::Home::BodyLeft,
		    LauncherUi::Home::BodyTop,
		    LauncherUi::Home::BodyRight,
		    LauncherUi::Home::BodyBottom);
		bodyLayout->setSpacing(LauncherUi::Home::SectionSpacing);

		const auto addHomeSection = [bodyLayout](const QString& title)
		{
			QLabel* section = new QLabel(title);
			section->setObjectName("CommandSectionTitle");
			section->setAccessibleName(title);
			bodyLayout->addWidget(section);
		};

		const QString heroTitle = "Explore Sparkle";
		const QString heroDetail = QStringLiteral(
		    "Choose Editor or Runtime below. Quick Start checks dependencies, syncs selected levels, builds, cooks missing content, and "
		    "launches automatically using startup level %1.")
		                               .arg(ResolveStartupLevelDisplayName());
		layout.addWidget(
		    CreateHomeHeroCard(m_repositoryRoot, heroTitle, heroDetail, "neutral", nullptr, nullptr, "showcase-hero.png", this));

		ResponsiveCardGridWidget* libraryGrid = new ResponsiveCardGridWidget(
		    LauncherUi::Home::ProductCardMinWidth,
		    LauncherUi::Home::ProductCardMaxWidth,
		    LauncherUi::Home::ProductCardMaxColumns,
		    LauncherUi::Home::TileSpacing,
		    LauncherUi::Home::TileSpacing,
		    quickStartBody);

		const bool editorReady = editorPlan.Result == LauncherCapabilityResolution::Kind::RunOperation && editorPlan.CompletesGoal;
		const bool editorBlocked = editorPlan.Result == LauncherCapabilityResolution::Kind::Blocked;
		const QString editorStatus = editorReady ? QStringLiteral("Ready")
		    : editorBlocked                      ? QStringLiteral("Needs attention")
		                                         : QStringLiteral("Automatic setup");
		const QString editorDetail = editorReady ? QStringLiteral("Launch immediately after a fresh readiness check.")
		    : editorBlocked ? QStringLiteral("Run Quick Start to see the exact prerequisite that needs attention in the activity log.")
		                    : QStringLiteral("Automatically sync, generate, build, and cook whatever the editor needs before launching.");
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Editor",
		    editorStatus,
		    editorDetail,
		    editorReady         ? "ok"
		        : editorBlocked ? "warning"
		                        : "neutral",
		    CreateQuickStartButton("launch.editor", "Run"),
		    "library",
		    "showcase-editor.png",
		    this));
		const bool runtimeReady = runtimePlan.Result == LauncherCapabilityResolution::Kind::RunOperation && runtimePlan.CompletesGoal;
		const bool runtimeBlocked = runtimePlan.Result == LauncherCapabilityResolution::Kind::Blocked;
		const QString runtimeStatus = runtimeReady ? QStringLiteral("Ready")
		    : runtimeBlocked                       ? QStringLiteral("Needs attention")
		                                           : QStringLiteral("Automatic setup");
		const QString runtimeDetail = runtimeReady ? QStringLiteral("Launch immediately after a fresh readiness check.")
		    : runtimeBlocked ? QStringLiteral("Run Quick Start to see the exact prerequisite that needs attention in the activity log.")
		                     : QStringLiteral("Automatically sync, generate, build, and cook whatever the runtime needs before launching.");
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Runtime",
		    runtimeStatus,
		    runtimeDetail,
		    runtimeReady         ? "ok"
		        : runtimeBlocked ? "warning"
		                         : "neutral",
		    CreateQuickStartButton("launch.runtime", "Run"),
		    "library",
		    "showcase-runtime.png",
		    this));
		if (libraryGrid->CardCount() > 0)
		{
			addHomeSection("Products");
			bodyLayout->addWidget(libraryGrid);
		}
		else
		{
			delete libraryGrid;
		}

		layout.addWidget(quickStartBody);
	}
}
