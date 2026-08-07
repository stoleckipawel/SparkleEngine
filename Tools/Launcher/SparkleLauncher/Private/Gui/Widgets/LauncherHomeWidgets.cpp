#include "LauncherHomeWidgets.h"

#include "LauncherLayoutWidgets.h"
#include "LauncherUiDesign.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QCoreApplication>
#include <QtCore/Qt>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>

#include <array>
#include <string>
#include <system_error>

namespace SparkleLauncher
{
	static constexpr int kSpaceSmall = LauncherUi::Space::Small;

	static QWidget* CreateLauncherVisualArtworkWidget(
	    const QPixmap& pixmap,
	    const QString& objectName,
	    const QSize& minimumSize,
	    LauncherArtworkPreset preset,
	    QWidget* parent);

	std::filesystem::path FindLauncherVisualAsset(const std::filesystem::path& repositoryRoot, const QString& fileName)
	{
		if (fileName.isEmpty())
		{
			return {};
		}

		const std::string assetName = fileName.toStdString();
		const std::filesystem::path applicationVisualPath =
		    std::filesystem::path(QCoreApplication::applicationDirPath().toStdString()) / "Visuals" / assetName;
		const std::array<std::filesystem::path, 5> candidates = {
		    repositoryRoot / "Tools" / "Launcher" / "SparkleLauncher" / "Assets" / "Visuals" / assetName,
		    applicationVisualPath,
		    GetArtifactDirectory(repositoryRoot) / "dev" / "launcher" / "Visuals" / assetName,
		    GetArtifactDirectory(repositoryRoot) / "diagnostics" / "launcher-visual-assets" / assetName,
		    GetArtifactDirectory(repositoryRoot) / "diagnostics" / "launcher-visual-assets"
		        / (std::filesystem::path(assetName).stem().string() + ".png")};
		for (const std::filesystem::path& candidate : candidates)
		{
			std::error_code errorCode;
			if (std::filesystem::is_regular_file(candidate, errorCode) && !errorCode)
			{
				return candidate;
			}
		}

		return {};
	}

	QWidget* CreateLauncherVisualArtworkWidget(
	    const std::filesystem::path& repositoryRoot,
	    const QString& fileName,
	    const QString& objectName,
	    const QSize& minimumSize,
	    LauncherArtworkPreset preset,
	    QWidget* parent)
	{
		const std::filesystem::path artworkPath = FindLauncherVisualAsset(repositoryRoot, fileName);
		if (artworkPath.empty())
		{
			return nullptr;
		}

		QPixmap pixmap(QString::fromStdString(artworkPath.string()));
		if (pixmap.isNull())
		{
			return nullptr;
		}

		QWidget* artwork = CreateLauncherVisualArtworkWidget(pixmap, objectName, minimumSize, preset, parent);
		artwork->setAccessibleName(QStringLiteral("Visual artwork: %1").arg(fileName));
		return artwork;
	}

	static QWidget* CreateLauncherVisualArtworkWidget(
	    const QPixmap& pixmap,
	    const QString& objectName,
	    const QSize& minimumSize,
	    LauncherArtworkPreset preset,
	    QWidget* parent)
	{
		if (pixmap.isNull())
		{
			return nullptr;
		}

		const QSize artworkSize = minimumSize.isEmpty() ? LauncherUi::WorkflowVisual::FallbackArtworkSize : minimumSize;
		LauncherArtworkWidget* artwork = new LauncherArtworkWidget(pixmap, LauncherArtworkSpec::ForPreset(preset), artworkSize, parent);
		artwork->setObjectName(objectName);
		QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		artwork->setSizePolicy(policy);
		artwork->setAccessibleName("Visual artwork");
		return artwork;
	}

	QFrame* CreateHomeHeroCard(
	    const std::filesystem::path& repositoryRoot,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* primaryAction,
	    QWidget* secondaryAction,
	    const QString& artworkFileName,
	    QWidget* parent)
	{
		QPixmap heroPixmap;
		const std::filesystem::path artworkPath = FindLauncherVisualAsset(repositoryRoot, artworkFileName);
		if (!artworkPath.empty())
		{
			heroPixmap.load(QString::fromStdString(artworkPath.string()));
		}

		HomeHeroCardWidget* card = new HomeHeroCardWidget(heroPixmap, parent);
		card->setObjectName("CommandHeroCard");
		card->setProperty("State", state);

		QWidget* copyPane = new QWidget(card);
		copyPane->setObjectName("CommandHeroCopyPane");
		QVBoxLayout* layout = new QVBoxLayout(copyPane);
		layout->setContentsMargins(0, 0, 0, 0);
		layout->setSpacing(LauncherUi::Hero::CopySpacing);

		QLabel* title = new QLabel(status, card);
		title->setObjectName("CommandHeroTitle");
		layout->addWidget(title);

		QLabel* body = new QLabel(detail, card);
		body->setObjectName("CommandHeroText");
		body->setWordWrap(true);
		layout->addWidget(body);

		QHBoxLayout* actionRow = new QHBoxLayout();
		actionRow->setContentsMargins(0, LauncherUi::Hero::ActionTopMargin, 0, 0);
		actionRow->setSpacing(kSpaceSmall);
		if (primaryAction != nullptr)
		{
			primaryAction->setParent(copyPane);
			actionRow->addWidget(primaryAction, 0, Qt::AlignLeft);
		}
		if (secondaryAction != nullptr)
		{
			secondaryAction->setParent(copyPane);
			actionRow->addWidget(secondaryAction, 0, Qt::AlignLeft);
		}
		actionRow->addStretch(1);
		layout->addLayout(actionRow);
		layout->addStretch(1);

		card->SetCopyPane(copyPane);
		return card;
	}

}
