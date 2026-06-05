#include "LauncherLayoutWidgets.h"

#include "LauncherArtworkWidgets.h"
#include "LauncherUiDesign.h"

#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QResizeEvent>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QLayout>
#include <QtWidgets/QScrollArea>

#include <algorithm>
#include <cmath>

namespace SparkleLauncher
{
	ProportionalCardFrame::ProportionalCardFrame(double aspectRatio, QWidget* parent)
	    : QFrame(parent)
	    , m_aspectRatio(aspectRatio > 0.0 ? aspectRatio : 16.0 / 9.0)
	{
		QSizePolicy policy(QSizePolicy::Fixed, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		setSizePolicy(policy);
	}

	void ProportionalCardFrame::SetActivationButton(QAbstractButton* button)
	{
		m_activationButton = button;
		const bool interactive = m_activationButton != nullptr;
		setCursor(interactive ? Qt::PointingHandCursor : Qt::ArrowCursor);
		setFocusPolicy(interactive ? Qt::StrongFocus : Qt::NoFocus);
		setProperty("Interactive", interactive ? "true" : "false");
		if (m_activationButton != nullptr)
		{
			m_activationButton->setFocusPolicy(Qt::NoFocus);
		}
	}

	bool ProportionalCardFrame::hasHeightForWidth() const
	{
		return true;
	}

	int ProportionalCardFrame::heightForWidth(int width) const
	{
		return std::max(1, static_cast<int>(std::round(static_cast<double>(std::max(1, width)) / m_aspectRatio)));
	}

	void ProportionalCardFrame::mouseReleaseEvent(QMouseEvent* event)
	{
		if (m_activationButton != nullptr && event != nullptr && event->button() == Qt::LeftButton && rect().contains(event->pos()))
		{
			m_activationButton->click();
			event->accept();
			return;
		}

		QFrame::mouseReleaseEvent(event);
	}

	void ProportionalCardFrame::keyPressEvent(QKeyEvent* event)
	{
		if (m_activationButton != nullptr && event != nullptr && (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space))
		{
			m_activationButton->click();
			event->accept();
			return;
		}

		QFrame::keyPressEvent(event);
	}

	HomeHeroCardWidget::HomeHeroCardWidget(QPixmap source, QWidget* parent)
	    : QFrame(parent)
	    , m_source(std::move(source))
	{
		setAttribute(Qt::WA_StyledBackground, false);
		setAttribute(Qt::WA_OpaquePaintEvent, false);
		setMinimumHeight(LauncherUi::Hero::MinimumHeight);
		setMaximumHeight(QWIDGETSIZE_MAX);
		QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		setSizePolicy(policy);
	}

	void HomeHeroCardWidget::SetCopyPane(QWidget* copyPane)
	{
		m_copyPane = copyPane;
		if (m_copyPane != nullptr)
		{
			m_copyPane->setParent(this);
			m_copyPane->setMaximumWidth(QWIDGETSIZE_MAX);
			m_copyPane->show();
			LayoutCopyPane();
		}
	}

	bool HomeHeroCardWidget::hasHeightForWidth() const
	{
		return true;
	}

	int HomeHeroCardWidget::heightForWidth(int width) const
	{
		const int proportionalHeight = static_cast<int>(std::round(static_cast<double>(width) * LauncherUi::Hero::DesignHeight / LauncherUi::Hero::DesignWidth));
		return std::max(proportionalHeight, LauncherUi::Hero::MinimumHeight);
	}

	QSize HomeHeroCardWidget::sizeHint() const
	{
		return QSize(LauncherUi::Hero::DesignWidth, LauncherUi::Hero::DesignHeight);
	}

	QSize HomeHeroCardWidget::minimumSizeHint() const
	{
		return QSize(LauncherUi::Hero::MinimumWidth, LauncherUi::Hero::MinimumHeight);
	}

	void HomeHeroCardWidget::paintEvent(QPaintEvent*)
	{
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.fillRect(rect(), LauncherUi::Color::Hex(LauncherUi::Color::Background));

		if (m_source.isNull() || width() <= 0 || height() <= 0)
		{
			return;
		}

		QImage heroLayer(QSize(LauncherUi::Hero::DesignWidth, LauncherUi::Hero::DesignHeight), QImage::Format_ARGB32_Premultiplied);
		heroLayer.fill(Qt::transparent);
		QPainter heroPainter(&heroLayer);
		heroPainter.setRenderHint(QPainter::Antialiasing, true);
		heroPainter.setRenderHint(QPainter::SmoothPixmapTransform, true);

		const QRect designRect(0, 0, LauncherUi::Hero::DesignWidth, LauncherUi::Hero::DesignHeight);
		PaintLauncherArtwork(
		    heroPainter,
		    designRect,
		    m_source,
		    LauncherArtworkSpec::ForPreset(LauncherArtworkPreset::HeroPanorama));

		painter.drawImage(HeroSceneRect(), heroLayer);
	}

	void HomeHeroCardWidget::resizeEvent(QResizeEvent* event)
	{
		QFrame::resizeEvent(event);
		SyncProportionalHeight();
		LayoutCopyPane();
	}

	void HomeHeroCardWidget::SyncProportionalHeight()
	{
		if (width() <= 0)
		{
			return;
		}

		const int desiredHeight = heightForWidth(width());
		if (minimumHeight() == desiredHeight && maximumHeight() == desiredHeight)
		{
			return;
		}

		setMinimumHeight(desiredHeight);
		setMaximumHeight(desiredHeight);
		updateGeometry();
	}

	void HomeHeroCardWidget::LayoutCopyPane()
	{
		if (m_copyPane == nullptr || width() <= 0 || height() <= 0)
		{
			return;
		}

		const double scale = std::clamp(
		    HeroSceneScale(),
		    LauncherUi::Hero::MinimumCopyScale,
		    LauncherUi::Hero::MaximumCopyScale);
		const QRectF sceneRect = HeroSceneRect();
		const int left = static_cast<int>(std::round(sceneRect.left() + LauncherUi::Hero::CopyLeft * scale));
		const int top = static_cast<int>(std::round(sceneRect.top() + LauncherUi::Hero::CopyTop * scale));
		const int bottom = static_cast<int>(std::round(LauncherUi::Hero::CopyBottom * scale));
		const int desiredPaneWidth = static_cast<int>(std::round(LauncherUi::Hero::CopyWidth * scale));
		const int maximumPaneWidth = std::max(300, static_cast<int>(std::round((LauncherUi::Hero::CopyDividerX - LauncherUi::Hero::CopyLeft - 54) * scale)));
		const int paneWidth = std::clamp(desiredPaneWidth, 300, std::min(460, maximumPaneWidth));
		const int paneHeight = std::max(140, static_cast<int>(std::round(sceneRect.height())) - static_cast<int>(std::round(LauncherUi::Hero::CopyTop * scale)) - bottom);

		if (QLayout* copyLayout = m_copyPane->layout())
		{
			copyLayout->setContentsMargins(0, 0, 0, 0);
			copyLayout->setSpacing(std::max(10, static_cast<int>(std::round(18 * scale))));
		}
		m_copyPane->setGeometry(left, top, paneWidth, paneHeight);
	}

	QRectF HomeHeroCardWidget::HeroSceneRect() const
	{
		if (width() <= 0 || height() <= 0)
		{
			return QRectF();
		}

		const double scale = HeroSceneScale();
		const QSizeF sceneSize(LauncherUi::Hero::DesignWidth * scale, LauncherUi::Hero::DesignHeight * scale);
		const QPointF sceneTopLeft(
		    0.0,
		    (static_cast<double>(height()) - sceneSize.height()) * 0.5);
		return QRectF(sceneTopLeft, sceneSize);
	}

	double HomeHeroCardWidget::HeroSceneScale() const
	{
		if (width() <= 0 || height() <= 0)
		{
			return 1.0;
		}
		return std::min(static_cast<double>(width()) / LauncherUi::Hero::DesignWidth, static_cast<double>(height()) / LauncherUi::Hero::DesignHeight);
	}

	ResponsiveCardGridWidget::ResponsiveCardGridWidget(
	    int minimumCardWidth,
	    int maximumCardWidth,
	    int maximumColumns,
	    int horizontalSpacing,
	    int verticalSpacing,
	    QWidget* parent)
	    : QWidget(parent)
	    , m_minimumCardWidth(std::max(1, minimumCardWidth))
	    , m_maximumCardWidth(std::max(m_minimumCardWidth, maximumCardWidth))
	    , m_maximumColumns(std::max(1, maximumColumns))
	{
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		m_layout = new QGridLayout(this);
		m_layout->setContentsMargins(0, 0, 0, 0);
		m_layout->setHorizontalSpacing(horizontalSpacing);
		m_layout->setVerticalSpacing(verticalSpacing);
		m_layout->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		m_layout->setSizeConstraint(QLayout::SetNoConstraint);
	}

	void ResponsiveCardGridWidget::AddCard(QWidget* card)
	{
		if (card == nullptr)
		{
			return;
		}

		card->setParent(this);
		card->setSizePolicy(QSizePolicy::Fixed, card->sizePolicy().verticalPolicy());
		m_cards.push_back(card);
		Reflow();
	}

	int ResponsiveCardGridWidget::CardCount() const
	{
		return static_cast<int>(m_cards.size());
	}

	QSize ResponsiveCardGridWidget::minimumSizeHint() const
	{
		const QSize layoutMinimum = m_layout != nullptr ? m_layout->minimumSize() : QWidget::minimumSizeHint();
		return QSize(m_minimumCardWidth, std::max(1, layoutMinimum.height()));
	}

	QSize ResponsiveCardGridWidget::sizeHint() const
	{
		const QSize layoutHint = m_layout != nullptr ? m_layout->sizeHint() : QWidget::sizeHint();
		return QSize(std::min(m_maximumCardWidth, std::max(m_minimumCardWidth, AvailableLayoutWidth())), std::max(1, layoutHint.height()));
	}

	void ResponsiveCardGridWidget::resizeEvent(QResizeEvent* event)
	{
		QWidget::resizeEvent(event);
		Reflow();
	}

	int ResponsiveCardGridWidget::AvailableLayoutWidth() const
	{
		int availableWidth = std::max(1, contentsRect().width());
		for (const QWidget* ancestor = parentWidget(); ancestor != nullptr; ancestor = ancestor->parentWidget())
		{
			if (const QScrollArea* scrollArea = qobject_cast<const QScrollArea*>(ancestor))
			{
				availableWidth = std::min(availableWidth, std::max(1, scrollArea->viewport()->contentsRect().width()));
				break;
			}
		}
		return std::max(1, availableWidth);
	}

	int ResponsiveCardGridWidget::DesiredColumnCount() const
	{
		const int spacing = std::max(0, m_layout->horizontalSpacing());
		const int availableWidth = AvailableLayoutWidth();
		const int columnsByWidth = std::max(1, (availableWidth + spacing) / (m_minimumCardWidth + spacing));
		return std::clamp(columnsByWidth, 1, std::min(m_maximumColumns, std::max(1, static_cast<int>(m_cards.size()))));
	}

	int ResponsiveCardGridWidget::DesiredCardWidth(int columns) const
	{
		const int spacing = std::max(0, m_layout->horizontalSpacing());
		const int availableWidth = AvailableLayoutWidth();
		const int availableForCards = std::max(1, availableWidth - (std::max(1, columns) - 1) * spacing);
		const int proportionalWidth = std::max(1, availableForCards / std::max(1, columns));
		const int lowerBound = std::min(m_minimumCardWidth, proportionalWidth);
		return std::clamp(proportionalWidth, lowerBound, m_maximumCardWidth);
	}

	void ResponsiveCardGridWidget::Reflow()
	{
		if (m_cards.empty())
		{
			return;
		}

		const int columns = DesiredColumnCount();
		const int cardWidth = DesiredCardWidth(columns);
		if (columns == m_currentColumns && cardWidth == m_currentCardWidth && m_layout->count() == static_cast<int>(m_cards.size()))
		{
			return;
		}

		while (QLayoutItem* item = m_layout->takeAt(0))
		{
			delete item;
		}

		for (int column = 0; column < std::max(m_currentColumns, columns) + 1; ++column)
		{
			m_layout->setColumnStretch(column, column == columns ? 1 : 0);
			m_layout->setColumnMinimumWidth(column, column < columns ? cardWidth : 0);
		}

		for (int index = 0; index < static_cast<int>(m_cards.size()); ++index)
		{
			m_cards[index]->setMinimumWidth(cardWidth);
			m_cards[index]->setMaximumWidth(cardWidth);
			if (m_cards[index]->hasHeightForWidth())
			{
				const int cardHeight = m_cards[index]->heightForWidth(cardWidth);
				m_cards[index]->setMinimumHeight(cardHeight);
				m_cards[index]->setMaximumHeight(cardHeight);
			}
			m_layout->addWidget(m_cards[index], index / columns, index % columns, Qt::AlignLeft | Qt::AlignTop);
		}

		m_currentColumns = columns;
		m_currentCardWidth = cardWidth;
		updateGeometry();
	}
}
