#include "LauncherMainWindow.h"

#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherQuickStartPlanner.h"
#include "LauncherUiDesign.h"

#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <utility>

namespace SparkleLauncher
{
	struct QuickStartCardPresentation final
	{
		QString Status;
		QString Detail;
		QString State;
	};

	static QuickStartCardPresentation BuildQuickStartCardPresentation(
	    const LauncherCapabilityResolution& resolution,
	    QString readyDetail,
	    QString automaticDetail,
	    QString blockedDetail)
	{
		const bool ready = resolution.Result == LauncherCapabilityResolution::Kind::RunOperation && resolution.CompletesGoal;
		if (ready)
		{
			return {"Ready", std::move(readyDetail), "ok"};
		}
		if (resolution.Result == LauncherCapabilityResolution::Kind::Blocked)
		{
			return {"Needs attention", std::move(blockedDetail), "warning"};
		}
		return {"Automatic setup", std::move(automaticDetail), "neutral"};
	}

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
		const LauncherCapabilityResolution openIdePlan = planQuickStart("workspace.open-ide");
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
		    "Run Editor, Runtime, or open your selected IDE below. Quick Start checks registered dependencies and performs every required "
		    "step automatically using startup level %1.")
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

		const QuickStartCardPresentation editorPresentation = BuildQuickStartCardPresentation(
		    editorPlan,
		    "Launch immediately after a fresh readiness check.",
		    "Automatically sync, generate, build, and cook whatever the editor needs before launching.",
		    "Run Quick Start to see the exact prerequisite that needs attention in the activity log.");
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Editor",
		    editorPresentation.Status,
		    editorPresentation.Detail,
		    editorPresentation.State,
		    CreateQuickStartButton("launch.editor", "Run"),
		    "library",
		    "showcase-editor.png",
		    this));
		const QuickStartCardPresentation runtimePresentation = BuildQuickStartCardPresentation(
		    runtimePlan,
		    "Launch immediately after a fresh readiness check.",
		    "Automatically sync, generate, build, and cook whatever the runtime needs before launching.",
		    "Run Quick Start to see the exact prerequisite that needs attention in the activity log.");
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    m_repositoryRoot,
		    "Runtime",
		    runtimePresentation.Status,
		    runtimePresentation.Detail,
		    runtimePresentation.State,
		    CreateQuickStartButton("launch.runtime", "Run"),
		    "library",
		    "showcase-runtime.png",
		    this));

		const QString selectedIdeName = ResolveSelectedWorkspaceIdeName(m_settings);
		const QuickStartCardPresentation openIdePresentation = BuildQuickStartCardPresentation(
		    openIdePlan,
		    QStringLiteral("Open %1 immediately after a fresh workspace check.").arg(selectedIdeName),
		    QStringLiteral("Automatically sync dependencies and generate current workspace files before opening %1.").arg(selectedIdeName),
		    QStringLiteral("Run Quick Start to see what %1 needs in the activity log.").arg(selectedIdeName));
		libraryGrid->AddCard(CreateHomeCapabilityCard(
		    "Open IDE",
		    openIdePresentation.Status,
		    openIdePresentation.Detail,
		    openIdePresentation.State,
		    CreateQuickStartButton("workspace.open-ide", "Open"),
		    "library",
		    CreateIdeQuickStartArtwork(m_repositoryRoot, ResolveSelectedWorkspaceIde(m_settings)),
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
