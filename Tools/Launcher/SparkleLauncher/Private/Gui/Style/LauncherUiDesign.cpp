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
}
