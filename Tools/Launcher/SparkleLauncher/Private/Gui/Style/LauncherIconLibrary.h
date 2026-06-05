#pragma once

#include <QtGui/QColor>
#include <QtGui/QIcon>
#include <QtCore/QString>

namespace SparkleLauncher
{
	enum class LauncherIcon
	{
		Start,
		Sync,
		Build,
		Cook,
		Run,
		Package,
		Maintain,
		Queued,
		Running,
		Done,
		Failed,
		Copy,
		Overflow,
	};

	class LauncherIconLibrary
	{
	public:
		void Load();
		QIcon ApplicationIcon() const;
		QIcon Icon(LauncherIcon icon, const QColor& color) const;

	private:
		QString Glyph(LauncherIcon icon) const;

		QString m_iconFontFamily;
	};
}
