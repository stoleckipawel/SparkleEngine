#include "LauncherIconLibrary.h"

#include "LauncherUiDesign.h"

#include <QtCore/QStringList>
#include <QtGui/QFont>
#include <QtGui/QFontDatabase>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace SparkleLauncher
{
	void LauncherIconLibrary::Load()
	{
#ifdef SPARKLE_FONT_AWESOME_SOLID_TTF
		const char* fontPath = SPARKLE_FONT_AWESOME_SOLID_TTF;
		std::error_code errorCode;
		if (!std::filesystem::exists(fontPath, errorCode) || errorCode)
		{
			return;
		}

		const int fontId = QFontDatabase::addApplicationFont(QString::fromUtf8(fontPath));
		if (fontId < 0)
		{
			return;
		}

		const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
		if (!families.empty())
		{
			m_iconFontFamily = families.front();
		}
#endif
	}

	QIcon LauncherIconLibrary::ApplicationIcon() const
	{
		QIcon icon;
		for (const int size : {16, 24, 32, 48, 64})
		{
			QPixmap pixmap(size, size);
			pixmap.fill(Qt::transparent);

			QPainter painter(&pixmap);
			painter.setRenderHint(QPainter::Antialiasing, true);
			const QRectF bounds(1.0, 1.0, size - 2.0, size - 2.0);
			const qreal radius = std::max(3.0, size * 0.18);
			painter.setPen(QColor("#3f4d35"));
			painter.setBrush(QColor("#151713"));
			painter.drawRoundedRect(bounds, radius, radius);

			painter.setPen(Qt::NoPen);
			painter.setBrush(QColor("#76b900"));
			painter.drawRoundedRect(QRectF(size * 0.18, size * 0.18, size * 0.64, size * 0.16), radius * 0.45, radius * 0.45);
			painter.setBrush(QColor("#dff3cf"));
			painter.drawEllipse(QRectF(size * 0.62, size * 0.62, size * 0.18, size * 0.18));

			QFont font("Segoe UI");
			font.setBold(true);
			font.setPixelSize(std::max(10, static_cast<int>(size * 0.48)));
			painter.setFont(font);
			painter.setPen(QColor("#f0f3f6"));
			painter.drawText(QRectF(0, size * 0.16, size, size * 0.72), Qt::AlignCenter, "S");
			icon.addPixmap(pixmap);
		}

		return icon;
	}

	QIcon LauncherIconLibrary::Icon(LauncherIcon icon, const QColor& color) const
	{
		if (m_iconFontFamily.isEmpty())
		{
			return {};
		}

		QPixmap pixmap(LauncherUi::Icon::DefaultSize, LauncherUi::Icon::DefaultSize);
		pixmap.fill(Qt::transparent);

		QPainter painter(&pixmap);
		painter.setRenderHint(QPainter::Antialiasing, true);
		painter.setRenderHint(QPainter::TextAntialiasing, true);
		QFont iconFont(m_iconFontFamily);
		iconFont.setPixelSize(LauncherUi::Icon::DefaultSize - 1);
		painter.setFont(iconFont);
		painter.setPen(color);
		painter.drawText(pixmap.rect(), Qt::AlignCenter, Glyph(icon));
		return QIcon(pixmap);
	}

	QString LauncherIconLibrary::Glyph(LauncherIcon icon) const
	{
		switch (icon)
		{
		case LauncherIcon::Start:
			return QChar(0xf135);
		case LauncherIcon::Sync:
			return QChar(0xf0ad);
		case LauncherIcon::Build:
			return QChar(0xf6e3);
		case LauncherIcon::Cook:
			return QChar(0xf466);
		case LauncherIcon::Run:
			return QChar(0xf04b);
		case LauncherIcon::Package:
			return QChar(0xf466);
		case LauncherIcon::Maintain:
			return QChar(0xf1de);
		case LauncherIcon::Queued:
			return QChar(0xf017);
		case LauncherIcon::Running:
			return QChar(0xf04b);
		case LauncherIcon::Done:
			return QChar(0xf00c);
		case LauncherIcon::Failed:
			return QChar(0xf071);
		case LauncherIcon::Copy:
			return QChar(0xf0c5);
		case LauncherIcon::Overflow:
			return QChar(0xf142);
		}

		return QString();
	}
}
