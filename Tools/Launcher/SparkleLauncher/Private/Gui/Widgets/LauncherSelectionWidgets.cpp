#include "LauncherSelectionWidgets.h"

#include "LauncherUiDesign.h"

#include <QtCore/QSignalBlocker>
#include <QtCore/Qt>
#include <QtGui/QBrush>
#include <QtGui/QColor>
#include <QtGui/QFont>
#include <QtGui/QStandardItem>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QComboBox>

#include <algorithm>

namespace SparkleLauncher
{
	static QStandardItem* ComboItem(QComboBox& combo, int row)
	{
		QStandardItemModel* model = qobject_cast<QStandardItemModel*>(combo.model());
		return model == nullptr ? nullptr : model->item(row);
	}

	static void AppendGroupHeading(QComboBox& combo, const QString& label)
	{
		combo.addItem(label);
		const int row = combo.count() - 1;
		combo.setItemData(row, label + " options", Qt::AccessibleTextRole);
		if (QStandardItem* item = ComboItem(combo, row))
		{
			QFont font = item->font();
			font.setBold(true);
			item->setFont(font);
			item->setForeground(QBrush(QColor(LauncherUi::Color::TextSecondary)));
			item->setFlags(Qt::NoItemFlags);
		}
	}

	static int AppendOption(QComboBox& combo, const LauncherSelectionOption& option)
	{
		combo.addItem(option.DisplayName, option.Value);
		const int row = combo.count() - 1;
		const QString availability = option.Available ? QStringLiteral("Available") : QStringLiteral("Supported, setup required");
		const QString detail = option.Detail.isEmpty() ? availability : availability + ". " + option.Detail;
		combo.setItemData(row, detail, Qt::ToolTipRole);
		combo.setItemData(row, option.DisplayName, Qt::AccessibleTextRole);
		combo.setItemData(row, detail, Qt::AccessibleDescriptionRole);
		if (!option.Available)
		{
			if (QStandardItem* item = ComboItem(combo, row))
			{
				item->setEnabled(false);
				item->setForeground(QBrush(QColor(LauncherUi::Color::TextMuted)));
			}
		}
		return row;
	}

	QString PopulateLauncherSelectionCombo(QComboBox& combo, const QVector<LauncherSelectionOption>& options, const QString& preferredValue)
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		combo.setPlaceholderText("None available");

		int selectedIndex = -1;
		int firstAvailableIndex = -1;
		const auto appendOptions = [&](bool available)
		{
			for (const LauncherSelectionOption& option : options)
			{
				if (option.Available != available)
				{
					continue;
				}

				const int row = AppendOption(combo, option);
				if (available && firstAvailableIndex < 0)
				{
					firstAvailableIndex = row;
				}
				if (available && option.Value == preferredValue)
				{
					selectedIndex = row;
				}
			}
		};

		AppendGroupHeading(combo, "Available");
		appendOptions(true);
		const bool hasSupportedOptions =
		    std::any_of(options.begin(), options.end(), [](const LauncherSelectionOption& option) { return !option.Available; });
		if (hasSupportedOptions)
		{
			combo.insertSeparator(combo.count());
			AppendGroupHeading(combo, "Supported");
			appendOptions(false);
		}

		selectedIndex = selectedIndex >= 0 ? selectedIndex : firstAvailableIndex;
		combo.setCurrentIndex(selectedIndex);
		combo.setEnabled(!options.empty());
		const int availableCount = static_cast<int>(
		    std::count_if(options.begin(), options.end(), [](const LauncherSelectionOption& option) { return option.Available; }));
		combo.setAccessibleDescription(
		    QStringLiteral("%1 available; %2 supported but unavailable.").arg(availableCount).arg(options.size() - availableCount));
		const int contentWidth = combo.view()->sizeHintForColumn(0) + LauncherUi::Selector::PopupHorizontalPadding;
		combo.view()->setMinimumWidth(qBound(combo.minimumWidth(), contentWidth, LauncherUi::Selector::PopupMaxWidth));
		return selectedIndex < 0 ? QString() : combo.currentData().toString();
	}
}
