#include "LauncherMainWindow.h"

#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherUiDesign.h"

#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
		m_quickStartButtons.clear();
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

		libraryGrid->AddCard(
		    CreateQuickStartCard(m_repositoryRoot, "Editor", CreateQuickStartButton("launch.editor", "Run"), "showcase-editor.png", this));
		libraryGrid->AddCard(CreateQuickStartCard(
		    m_repositoryRoot,
		    "Runtime",
		    CreateQuickStartButton("launch.runtime", "Run"),
		    "showcase-runtime.png",
		    this));

		libraryGrid->AddCard(CreateQuickStartCard(
		    "Open IDE",
		    CreateQuickStartButton("workspace.open-ide", "Open"),
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
