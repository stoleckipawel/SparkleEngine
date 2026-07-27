#include "LauncherHomeWidgets.h"

#include "LauncherLayoutWidgets.h"
#include "LauncherUiDesign.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QCoreApplication>
#include <QtCore/Qt>
#include <QtGui/QPixmap>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>

#include <array>
#include <system_error>

namespace SparkleLauncher
{

		constexpr int kSpaceSmall = LauncherUi::Space::Small;


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
		    GetArtifactDirectory(repositoryRoot) / "diagnostics" / "launcher-visual-assets" / (std::filesystem::path(assetName).stem().string() + ".png")};
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

		const QSize artworkSize = minimumSize.isEmpty() ? LauncherUi::WorkflowVisual::FallbackArtworkSize : minimumSize;
		LauncherArtworkWidget* artwork = new LauncherArtworkWidget(pixmap, LauncherArtworkSpec::ForPreset(preset), artworkSize, parent);
		artwork->setObjectName(objectName);
		artwork->setMinimumSize(QSize(artworkSize.width(), artwork->heightForWidth(artworkSize.width())));
		QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		artwork->setSizePolicy(policy);
		artwork->setAccessibleName(QStringLiteral("Visual artwork: %1").arg(fileName));
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

	QFrame* CreateHomeCapabilityCard(
	    const std::filesystem::path& repositoryRoot,
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QString& artworkFileName,
	    QWidget* parent)
	{
		ProportionalCardFrame* card = new ProportionalCardFrame(LauncherUi::Card::HomeTileAspectRatio, parent);
		card->setObjectName("CommandCapabilityCard");
		card->setProperty("State", state);
		card->setProperty("TileRole", tileRole);

		const bool hasArtwork = !FindLauncherVisualAsset(repositoryRoot, artworkFileName).empty();
		const bool isLibraryCard = tileRole == "library";
		const bool isDiscoverCard = tileRole == "discover";
		const bool flushArtwork = hasArtwork;

		QVBoxLayout* layout = new QVBoxLayout(card);
		layout->setContentsMargins(
		    flushArtwork ? LauncherUi::Card::FlushArtworkMargins :
		                   (isLibraryCard ? LauncherUi::Card::ProductMargins(hasArtwork) : LauncherUi::Card::DiscoverMargins(hasArtwork)));
		layout->setSpacing(flushArtwork ? 0 : (isLibraryCard ? LauncherUi::Card::ProductSpacing : LauncherUi::Card::DiscoverSpacing));

		const QSize artworkDesignSize = isLibraryCard ? LauncherUi::Card::ProductArtworkSize : LauncherUi::Card::DiscoverArtworkSize;
		const LauncherArtworkPreset artworkPreset =
		    isLibraryCard ? LauncherArtworkPreset::ProductCard : LauncherArtworkPreset::DiscoverTile;
		if (QWidget* artwork = CreateLauncherVisualArtworkWidget(repositoryRoot, artworkFileName, "CommandCardArtwork", artworkDesignSize, artworkPreset, card))
		{
			artwork->setProperty("TileRole", tileRole);
			QSizePolicy artworkPolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
			artworkPolicy.setHeightForWidth(true);
			artwork->setSizePolicy(artworkPolicy);
			layout->addWidget(artwork);
		}

		QVBoxLayout* contentLayout = layout;
		if (flushArtwork)
		{
			QWidget* body = new QWidget(card);
			body->setObjectName("CommandCardBody");
			QVBoxLayout* bodyLayout = new QVBoxLayout(body);
			bodyLayout->setContentsMargins(isDiscoverCard ? LauncherUi::Card::DiscoverBodyMargins : LauncherUi::Card::ProductBodyMargins);
			bodyLayout->setSpacing(isDiscoverCard ? LauncherUi::Card::DiscoverSpacing : LauncherUi::Card::ProductSpacing);
			layout->addWidget(body, 1);
			contentLayout = bodyLayout;
		}

		if (isDiscoverCard)
		{
			contentLayout->addStretch(1);
		}

		QHBoxLayout* titleRow = new QHBoxLayout();
		titleRow->setContentsMargins(0, 0, 0, 0);
		titleRow->setSpacing(kSpaceSmall);
		QLabel* titleLabel = new QLabel(title, card);
		titleLabel->setObjectName("CommandCardTitle");
		titleRow->addWidget(titleLabel, 1);
		if (!isDiscoverCard)
		{
			QLabel* statusLabel = new QLabel(status, card);
			statusLabel->setObjectName("CommandCardChip");
			statusLabel->setProperty("State", state);
			titleRow->addWidget(statusLabel, 0, Qt::AlignRight | Qt::AlignTop);
		}
		contentLayout->addLayout(titleRow);

		if (isDiscoverCard)
		{
			card->setToolTip(QString("%1 - %2").arg(status, detail));
			card->setAccessibleName(title);
			card->setAccessibleDescription(QString("%1. %2").arg(status, detail));
			if (QAbstractButton* activationButton = qobject_cast<QAbstractButton*>(action))
			{
				activationButton->setParent(card);
				activationButton->hide();
				card->SetActivationButton(activationButton);
			}
			else if (action != nullptr)
			{
				action->deleteLater();
			}
			return card;
		}

		QLabel* detailLabel = new QLabel(detail, card);
		detailLabel->setObjectName("CommandCardText");
		detailLabel->setWordWrap(true);
		contentLayout->addWidget(detailLabel, 1);

		if (action != nullptr)
		{
			action->setParent(card);
			contentLayout->addWidget(action, 0, Qt::AlignLeft);
		}
		return card;
	}
}
