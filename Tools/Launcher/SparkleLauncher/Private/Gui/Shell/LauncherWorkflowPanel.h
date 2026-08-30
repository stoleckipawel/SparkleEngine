#pragma once

#include "LauncherWorkflowCatalog.h"

#include <QtCore/QHash>
#include <QtCore/QVector>
#include <QtWidgets/QWidget>

#include <functional>

class QAbstractButton;
class QButtonGroup;
class QStackedWidget;

namespace SparkleLauncher
{
	class LauncherIconLibrary;

	class LauncherWorkflowPanel final : public QWidget
	{
		Q_OBJECT

	public:
		LauncherWorkflowPanel(
		    const LauncherIconLibrary& icons,
		    const std::function<QString(const QString&)>& displayNameForOperation,
		    const std::function<void(QWidget*)>& registerFocusable,
		    const std::function<QWidget*(QWidget*)>& createOptionsPanel,
		    QWidget* parent = nullptr);

		QString InitialOperationId() const;
		void SetSelectedOperation(const QString& operationId);

	signals:
		void OperationSelected(const QString& operationId);

	private:
		void SelectWorkflowGroup(QAbstractButton* button);
		void SetActiveWorkflowGroup(int workflowIndex);

		QVector<LauncherWorkflowDefinition> m_workflows;
		QButtonGroup* m_workflowGroupButtons = nullptr;
		QButtonGroup* m_operationButtons = nullptr;
		QStackedWidget* m_operationTabs = nullptr;
		QHash<QString, int> m_workflowPageByOperation;
		QHash<int, QString> m_lastOperationByWorkflowIndex;
	};
}
