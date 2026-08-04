#include "LauncherArtworkWidgets.h"

#include "LauncherUiDesign.h"

#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPaintEvent>

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <utility>

namespace SparkleLauncher
{
	static QColor SurfaceColor()
	{
		return LauncherUi::Color::Hex(LauncherUi::Color::Background);
	}

	static QColor HeroColor(int alpha = 255)
	{
		return LauncherUi::Color::Hex(LauncherUi::Color::HeroBackground, alpha);
	}

	static QColor AccentColor(int alpha = 255)
	{
		return LauncherUi::Color::Hex(LauncherUi::Color::Accent, alpha);
	}

	static QColor NeutralShade(int alpha)
	{
		return QColor(0, 0, 0, alpha);
	}

	static QColor SurfaceShade(int alpha)
	{
		const QColor surface = SurfaceColor();
		return QColor(surface.red(), surface.green(), surface.blue(), alpha);
	}

	static QRectF ResolveArea(const QRectF& targetRect, const QRectF& normalizedArea)
	{
		return QRectF(
		    targetRect.left() + targetRect.width() * normalizedArea.left(),
		    targetRect.top() + targetRect.height() * normalizedArea.top(),
		    targetRect.width() * normalizedArea.width(),
		    targetRect.height() * normalizedArea.height());
	}

	static QLinearGradient CreateGradient(const QRectF& area, LauncherArtworkGradientAxis axis)
	{
		switch (axis)
		{
			case LauncherArtworkGradientAxis::Vertical:
				return QLinearGradient(area.left(), area.top(), area.left(), area.bottom());
			case LauncherArtworkGradientAxis::DiagonalDown:
				return QLinearGradient(area.left(), area.top(), area.right(), area.bottom());
			case LauncherArtworkGradientAxis::Horizontal:
			default:
				return QLinearGradient(area.left(), area.top(), area.right(), area.top());
		}
	}

	static void AddLayer(
	    QVector<LauncherArtworkGradientLayer>& layers,
	    LauncherArtworkGradientAxis axis,
	    std::initializer_list<LauncherArtworkGradientStop> stops,
	    QRectF normalizedArea = QRectF(0.0, 0.0, 1.0, 1.0))
	{
		LauncherArtworkGradientLayer layer;
		layer.Axis = axis;
		layer.NormalizedArea = normalizedArea;
		layer.Stops.reserve(static_cast<int>(stops.size()));
		for (const LauncherArtworkGradientStop& stop : stops)
		{
			layer.Stops.push_back(stop);
		}
		layers.push_back(layer);
	}

	static void DrawCoverPixmap(QPainter& painter, const QRectF& targetRect, const QPixmap& source)
	{
		if (source.isNull() || targetRect.isEmpty())
		{
			return;
		}

		const QSize targetSize(
		    std::max(1, static_cast<int>(std::ceil(targetRect.width()))),
		    std::max(1, static_cast<int>(std::ceil(targetRect.height()))));
		const QPixmap scaled = source.scaled(targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		const QRect cropRect(
		    std::max(0, (scaled.width() - targetSize.width()) / 2),
		    std::max(0, (scaled.height() - targetSize.height()) / 2),
		    targetSize.width(),
		    targetSize.height());
		painter.drawPixmap(targetRect.toRect(), scaled, cropRect);
	}

	LauncherArtworkSpec LauncherArtworkSpec::ForPreset(LauncherArtworkPreset preset)
	{
		LauncherArtworkSpec spec;
		spec.BaseColor = HeroColor();
		spec.AspectRatio = 16.0 / 9.0;

		switch (preset)
		{
			case LauncherArtworkPreset::HeroPanorama:
				spec.AspectRatio = static_cast<double>(LauncherUi::Hero::DesignWidth) / static_cast<double>(LauncherUi::Hero::DesignHeight);
				spec.AccentLineX = static_cast<double>(LauncherUi::Hero::CopyDividerX) / static_cast<double>(LauncherUi::Hero::DesignWidth);
				spec.AccentLineColor = AccentColor(210);
				spec.AccentLineWidth = 2.0;
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Horizontal,
				    {
				        {0.00, NeutralShade(132)},
				        {0.52, NeutralShade(72)},
				        {1.00, NeutralShade(0)},
				    },
				    QRectF(0.0, 0.0, spec.AccentLineX, 1.0));
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Horizontal,
				    {
				        {0.00, NeutralShade(0)},
				        {1.00, NeutralShade(84)},
				    },
				    QRectF(0.80, 0.0, 0.20, 1.0));
				AddLayer(
				    spec.TopLayers,
				    LauncherArtworkGradientAxis::Vertical,
				    {
				        {0.00, SurfaceShade(0)},
				        {0.50, SurfaceShade(0)},
				        {0.70, SurfaceShade(50)},
				        {0.86, SurfaceShade(172)},
				        {1.00, SurfaceShade(255)},
				    });
				break;

			case LauncherArtworkPreset::WorkflowBanner:
				spec.AspectRatio = 2.4;
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Horizontal,
				    {
				        {0.00, NeutralShade(64)},
				        {0.30, NeutralShade(28)},
				        {0.74, NeutralShade(8)},
				        {1.00, NeutralShade(0)},
				    });
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Vertical,
				    {
				        {0.00, NeutralShade(0)},
				        {0.58, NeutralShade(0)},
				        {1.00, NeutralShade(56)},
				    });
				break;

			case LauncherArtworkPreset::ProductCard:
				spec.AspectRatio = static_cast<double>(LauncherUi::Card::ProductArtworkSize.width())
				    / static_cast<double>(LauncherUi::Card::ProductArtworkSize.height());
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Horizontal,
				    {
				        {0.00, NeutralShade(72)},
				        {0.30, NeutralShade(34)},
				        {0.72, NeutralShade(12)},
				        {0.92, NeutralShade(2)},
				        {1.00, NeutralShade(0)},
				    });
				AddLayer(
				    spec.Layers,
				    LauncherArtworkGradientAxis::Vertical,
				    {
				        {0.00, NeutralShade(0)},
				        {0.56, NeutralShade(0)},
				        {1.00, NeutralShade(72)},
				    });
				break;
		}

		return spec;
	}

	static void PaintLayers(QPainter& painter, const QRectF& targetRect, const QVector<LauncherArtworkGradientLayer>& layers)
	{
		for (const LauncherArtworkGradientLayer& layer : layers)
		{
			const QRectF area = ResolveArea(targetRect, layer.NormalizedArea);
			if (area.isEmpty())
			{
				continue;
			}

			QLinearGradient gradient = CreateGradient(area, layer.Axis);
			for (const LauncherArtworkGradientStop& stop : layer.Stops)
			{
				gradient.setColorAt(std::clamp(stop.Position, 0.0, 1.0), stop.Color);
			}
			painter.fillRect(area, gradient);
		}
	}

	void PaintLauncherArtwork(QPainter& painter, const QRectF& targetRect, const QPixmap& source, const LauncherArtworkSpec& spec)
	{
		if (targetRect.isEmpty())
		{
			return;
		}

		painter.save();
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
		painter.fillRect(targetRect, spec.BaseColor);
		DrawCoverPixmap(painter, targetRect, source);
		PaintLayers(painter, targetRect, spec.Layers);

		if (spec.AccentLineX >= 0.0)
		{
			const qreal x = targetRect.left() + targetRect.width() * std::clamp(spec.AccentLineX, 0.0, 1.0);
			painter.setPen(QPen(spec.AccentLineColor, spec.AccentLineWidth));
			painter.drawLine(QPointF(x, targetRect.top()), QPointF(x, targetRect.bottom() - 1.0));
		}
		PaintLayers(painter, targetRect, spec.TopLayers);
		painter.restore();
	}

	LauncherArtworkWidget::LauncherArtworkWidget(QPixmap source, LauncherArtworkSpec spec, QSize designSize, QWidget* parent) :
	    QWidget(parent),
	    m_source(std::move(source)),
	    m_spec(std::move(spec)),
	    m_designSize(designSize.isEmpty() ? QSize(360, 180) : designSize)
	{
		setAttribute(Qt::WA_StyledBackground, true);
		setAttribute(Qt::WA_OpaquePaintEvent, false);
		setMinimumSize(1, 1);
		QSizePolicy policy(QSizePolicy::Expanding, QSizePolicy::Preferred);
		policy.setHeightForWidth(true);
		setSizePolicy(policy);
	}

	bool LauncherArtworkWidget::hasHeightForWidth() const
	{
		return true;
	}

	int LauncherArtworkWidget::heightForWidth(int width) const
	{
		if (width <= 0 || m_spec.AspectRatio <= 0.0)
		{
			return QWidget::heightForWidth(width);
		}
		return std::max(1, static_cast<int>(std::round(static_cast<double>(width) / m_spec.AspectRatio)));
	}

	QSize LauncherArtworkWidget::sizeHint() const
	{
		return QSize(m_designSize.width(), heightForWidth(m_designSize.width()));
	}

	void LauncherArtworkWidget::paintEvent(QPaintEvent*)
	{
		QPainter painter(this);
		PaintLauncherArtwork(painter, rect(), m_source, m_spec);
	}
}
