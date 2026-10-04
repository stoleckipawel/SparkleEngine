#pragma once

#include "LauncherContextUiModel.h"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

#include <functional>

class QCheckBox;
class QComboBox;
class QLabel;
class QObject;

namespace SparkleLauncher
{
	QString PopulateLauncherSelectionCombo(
	    QComboBox& combo,
	    const QVector<LauncherSelectionOption>& options,
	    const QString& preferredValue);

	void ConnectSelectAllScopeBox(
	    QCheckBox* selectAllBox,
	    const QVector<QCheckBox*>& scopeBoxes,
	    QObject* context,
	    std::function<void(bool)> commitSelection);
	QStringList CollectSelectedScopeValues(const QVector<QCheckBox*>& scopeBoxes);
	void UpdateScopeSelectionSummary(QLabel* selectionSummary, bool hasSelection, const QString& emptyMessage);
}
