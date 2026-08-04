#include "LauncherHomeWidgets.h"

#include "LauncherLayoutWidgets.h"
#include "LauncherUiDesign.h"

#include "SparkleLauncher/LauncherPaths.h"

#include <QtCore/QCoreApplication>
#include <QtCore/Qt>
#include <QtGui/QFont>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QAbstractButton>
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

	static void DrawRiderMark(QPainter& painter, const QRect& bounds)
	{
		QLinearGradient gradient(bounds.topLeft(), bounds.bottomRight());
		gradient.setColorAt(0.0, QColor(255, 122, 0));
		gradient.setColorAt(0.46, QColor(255, 0, 141));
		gradient.setColorAt(1.0, QColor(85, 70, 255));
		painter.fillRect(bounds, gradient);

		const QRect center = bounds.adjusted(18, 18, -18, -18);
		painter.fillRect(center, QColor(10, 10, 12));
		QFont markFont = painter.font();
		markFont.setBold(true);
		markFont.setPixelSize(34);
		painter.setFont(markFont);
		painter.setPen(Qt::white);
		painter.drawText(center.adjusted(10, 6, -4, -14), Qt::AlignLeft | Qt::AlignVCenter, "RD");
		painter.fillRect(QRect(center.left() + 11, center.bottom() - 18, 34, 4), Qt::white);
	}

	static void DrawVisualStudioMark(QPainter& painter, const QRect& bounds)
	{
		const QPointF left(bounds.left() + 8, bounds.center().y());
		const QPointF top(bounds.left() + bounds.width() * 0.62, bounds.top() + 8);
		const QPointF right(bounds.right() - 8, bounds.center().y());
		const QPointF bottom(bounds.left() + bounds.width() * 0.62, bounds.bottom() - 8);
		QPainterPath ribbon;
		ribbon.moveTo(left);
		ribbon.lineTo(bounds.left() + bounds.width() * 0.35, bounds.top() + bounds.height() * 0.30);
		ribbon.lineTo(top);
		ribbon.lineTo(right);
		ribbon.lineTo(bottom);
		ribbon.lineTo(bounds.left() + bounds.width() * 0.35, bounds.bottom() - bounds.height() * 0.30);
		ribbon.closeSubpath();
		QLinearGradient gradient(bounds.topLeft(), bounds.bottomRight());
		gradient.setColorAt(0.0, QColor(177, 110, 255));
		gradient.setColorAt(1.0, QColor(92, 45, 184));
		painter.fillPath(ribbon, gradient);
		painter.setPen(QPen(QColor(223, 194, 255), 3));
		painter.drawPath(ribbon);

		QFont markFont = painter.font();
		markFont.setBold(true);
		markFont.setPixelSize(25);
		painter.setFont(markFont);
		painter.setPen(Qt::white);
		painter.drawText(bounds.adjusted(48, 0, -4, 0), Qt::AlignCenter, "VS");
	}

	QPixmap CreateIdeQuickStartArtwork(const std::filesystem::path& repositoryRoot, WorkspaceIde ide)
	{
		const bool riderSelected = ide == WorkspaceIde::Rider;
		QPixmap artwork(LauncherUi::Card::ProductArtworkSize);
		artwork.fill(QColor(16, 19, 18));
		QPainter painter(&artwork);
		const std::filesystem::path baseArtworkPath = FindLauncherVisualAsset(repositoryRoot, "workflow-open-ide.png");
		const QPixmap baseArtwork = baseArtworkPath.empty() ? QPixmap() : QPixmap(QString::fromStdString(baseArtworkPath.string()));
		PaintLauncherArtwork(painter, artwork.rect(), baseArtwork, LauncherArtworkSpec::ForPreset(LauncherArtworkPreset::ProductCard));

		QLinearGradient wash(0, 0, artwork.width(), 0);
		wash.setColorAt(0.0, QColor(8, 12, 10, 210));
		wash.setColorAt(0.52, QColor(15, 22, 18, 185));
		wash.setColorAt(1.0, riderSelected ? QColor(89, 36, 118, 225) : QColor(38, 45, 122, 225));
		painter.fillRect(artwork.rect(), wash);

		const QRect iconPanel(artwork.width() - 176, 22, 132, 132);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setBrush(QColor(13, 17, 15, 198));
		painter.setPen(QPen(QColor(112, 190, 0, 185), 2));
		painter.drawRoundedRect(iconPanel, 12, 12);
		const QRect markBounds = iconPanel.adjusted(26, 26, -26, -26);
		if (riderSelected)
		{
			DrawRiderMark(painter, markBounds);
		}
		else
		{
			DrawVisualStudioMark(painter, markBounds);
		}
		return artwork;
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

	static QFrame* CreateHomeCapabilityCardWithArtwork(
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QPixmap& artworkPixmap,
	    QWidget* parent)
	{
		ProportionalCardFrame* card = new ProportionalCardFrame(LauncherUi::Card::HomeTileAspectRatio, parent);
		card->setObjectName("CommandCapabilityCard");
		card->setProperty("State", state);
		card->setProperty("TileRole", tileRole);

		const bool hasArtwork = !artworkPixmap.isNull();
		const bool isLibraryCard = tileRole == "library";
		const bool isDiscoverCard = tileRole == "discover";
		const bool flushArtwork = hasArtwork;

		QVBoxLayout* layout = new QVBoxLayout(card);
		layout->setContentsMargins(
		    flushArtwork ? LauncherUi::Card::FlushArtworkMargins
		                 : (isLibraryCard ? LauncherUi::Card::ProductMargins(hasArtwork) : LauncherUi::Card::DiscoverMargins(hasArtwork)));
		layout->setSpacing(flushArtwork ? 0 : (isLibraryCard ? LauncherUi::Card::ProductSpacing : LauncherUi::Card::DiscoverSpacing));

		const QSize artworkDesignSize = isLibraryCard ? LauncherUi::Card::ProductArtworkSize : LauncherUi::Card::DiscoverArtworkSize;
		const LauncherArtworkPreset artworkPreset =
		    isLibraryCard ? LauncherArtworkPreset::ProductCard : LauncherArtworkPreset::DiscoverTile;
		if (QWidget* artwork =
		        CreateLauncherVisualArtworkWidget(artworkPixmap, "CommandCardArtwork", artworkDesignSize, artworkPreset, card))
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
		QPixmap artwork;
		const std::filesystem::path artworkPath = FindLauncherVisualAsset(repositoryRoot, artworkFileName);
		if (!artworkPath.empty())
		{
			artwork.load(QString::fromStdString(artworkPath.string()));
		}
		return CreateHomeCapabilityCardWithArtwork(title, status, detail, state, action, tileRole, artwork, parent);
	}

	QFrame* CreateHomeCapabilityCard(
	    const QString& title,
	    const QString& status,
	    const QString& detail,
	    const QString& state,
	    QWidget* action,
	    const QString& tileRole,
	    const QPixmap& artwork,
	    QWidget* parent)
	{
		return CreateHomeCapabilityCardWithArtwork(title, status, detail, state, action, tileRole, artwork, parent);
	}
}
