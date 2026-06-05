#pragma once

#include <QtCore/QPointer>
#include <QtCore/QVector>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QWidget>

class QAbstractButton;
class QGridLayout;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QResizeEvent;

namespace SparkleLauncher
{
	class ProportionalCardFrame final : public QFrame
	{
	public:
		explicit ProportionalCardFrame(double aspectRatio, QWidget* parent = nullptr);

		void SetActivationButton(QAbstractButton* button);

		bool hasHeightForWidth() const override;
		int heightForWidth(int width) const override;

	protected:
		void mouseReleaseEvent(QMouseEvent* event) override;
		void keyPressEvent(QKeyEvent* event) override;

	private:
		QPointer<QAbstractButton> m_activationButton;
		double m_aspectRatio = 16.0 / 9.0;
	};

	class HomeHeroCardWidget final : public QFrame
	{
	public:
		explicit HomeHeroCardWidget(QPixmap source, QWidget* parent = nullptr);

		void SetCopyPane(QWidget* copyPane);

		bool hasHeightForWidth() const override;
		int heightForWidth(int width) const override;
		QSize sizeHint() const override;
		QSize minimumSizeHint() const override;

	protected:
		void paintEvent(QPaintEvent* event) override;
		void resizeEvent(QResizeEvent* event) override;

	private:
		void SyncProportionalHeight();
		void LayoutCopyPane();
		QRectF HeroSceneRect() const;
		double HeroSceneScale() const;

		QPixmap m_source;
		QWidget* m_copyPane = nullptr;
	};

	class ResponsiveCardGridWidget final : public QWidget
	{
	public:
		ResponsiveCardGridWidget(
		    int minimumCardWidth,
		    int maximumCardWidth,
		    int maximumColumns,
		    int horizontalSpacing,
		    int verticalSpacing,
		    QWidget* parent = nullptr);

		void AddCard(QWidget* card);
		int CardCount() const;

		QSize minimumSizeHint() const override;
		QSize sizeHint() const override;

	protected:
		void resizeEvent(QResizeEvent* event) override;

	private:
		int AvailableLayoutWidth() const;
		int DesiredColumnCount() const;
		int DesiredCardWidth(int columns) const;
		void Reflow();

		QGridLayout* m_layout = nullptr;
		QVector<QWidget*> m_cards;
		int m_minimumCardWidth = 320;
		int m_maximumCardWidth = 420;
		int m_maximumColumns = 3;
		int m_currentColumns = 0;
		int m_currentCardWidth = 0;
	};
}
