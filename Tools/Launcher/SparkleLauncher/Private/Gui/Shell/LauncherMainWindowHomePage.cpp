#include "LauncherMainWindow.h"

#include "LauncherHomeWidgets.h"
#include "LauncherUiDesign.h"

#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddHomeQuickStart(QVBoxLayout& layout)
	{
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

		const QString heroTitle = "Explore Sparkle";
		const QString heroDetail = QStringLiteral(
		    "Choose a level below. Quick Start acquires missing content, prepares build and cook prerequisites, then runs that map.");
		layout.addWidget(
		    CreateHomeHeroCard(m_repositoryRoot, heroTitle, heroDetail, "neutral", nullptr, nullptr, "showcase-hero.png", this));

		AddSyncLevelContentGroups(*bodyLayout);
		layout.addWidget(quickStartBody);
	}
}
