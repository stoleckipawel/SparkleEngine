#pragma once

#include <QtCore/QRectF>
#include <QtCore/QSize>
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtGui/QPixmap>
#include <QtWidgets/QWidget>

#include <cstdint>

class QPainter;
class QPaintEvent;

namespace SparkleLauncher
{
	enum class LauncherArtworkPreset : std::uint8_t
	{
		HeroPanorama,
		WorkflowBanner,
	};

	enum class LauncherArtworkGradientAxis : std::uint8_t
	{
		Horizontal,
		Vertical,
		DiagonalDown,
	};

	struct LauncherArtworkGradientStop
	{
		qreal Position = 0.0;
		QColor Color;
	};

	struct LauncherArtworkGradientLayer
	{
		LauncherArtworkGradientAxis Axis = LauncherArtworkGradientAxis::Horizontal;
		QRectF NormalizedArea = QRectF(0.0, 0.0, 1.0, 1.0);
		QVector<LauncherArtworkGradientStop> Stops;
	};

	struct LauncherArtworkSpec
	{
		QColor BaseColor;
		double AspectRatio = 16.0 / 9.0;
		double AccentLineX = -1.0;
		qreal AccentLineWidth = 2.0;
		QColor AccentLineColor;
		QVector<LauncherArtworkGradientLayer> Layers;
		QVector<LauncherArtworkGradientLayer> TopLayers;

		static LauncherArtworkSpec ForPreset(LauncherArtworkPreset preset);
	};

	void PaintLauncherArtwork(QPainter& painter, const QRectF& targetRect, const QPixmap& source, const LauncherArtworkSpec& spec);

	class LauncherArtworkWidget final : public QWidget
	{
	public:
		LauncherArtworkWidget(QPixmap source, LauncherArtworkSpec spec, QSize designSize, QWidget* parent = nullptr);

		bool hasHeightForWidth() const override;
		int heightForWidth(int width) const override;
		QSize sizeHint() const override;

	protected:
		void paintEvent(QPaintEvent* event) override;

	private:
		QPixmap m_source;
		LauncherArtworkSpec m_spec;
		QSize m_designSize;
	};
}
