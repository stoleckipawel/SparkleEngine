#pragma once

#include "LauncherContextUiModel.h"

#include <QtCore/QString>
#include <QtCore/QVector>

class QComboBox;

namespace SparkleLauncher
{
	QString PopulateLauncherSelectionCombo(
	    QComboBox& combo,
	    const QVector<LauncherSelectionOption>& options,
	    const QString& preferredValue);
}
