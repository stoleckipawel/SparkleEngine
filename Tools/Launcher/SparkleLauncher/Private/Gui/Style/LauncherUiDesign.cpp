#include "LauncherUiDesign.h"

#include <QtCore/QString>

namespace SparkleLauncher::LauncherUi
{
	namespace Color
	{
		QColor Hex(const char* value, int alpha)
		{
			QColor color(QString::fromLatin1(value));
			color.setAlpha(alpha);
			return color;
		}
	}

	namespace Card
	{
		QMargins ProductMargins(bool hasArtwork)
		{
			return QMargins(18, hasArtwork ? 12 : 16, 18, 16);
		}

		QMargins DiscoverMargins(bool hasArtwork)
		{
			return QMargins(16, hasArtwork ? 12 : 14, 16, 14);
		}
	}
}
