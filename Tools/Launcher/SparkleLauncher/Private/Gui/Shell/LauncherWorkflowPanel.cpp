#include "LauncherWorkflowPanel.h"

#include "LauncherIconLibrary.h"
#include "LauncherUiDesign.h"

#include <QtCore/QtGlobal>
#include <QtGui/QColor>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStyle>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>

namespace SparkleLauncher
{
	static QIcon WorkflowIcon(const LauncherIconLibrary& icons, LauncherWorkflowPageKind pageKind)
	{
		LauncherIcon icon = LauncherIcon::Start;
		switch (pageKind)
		{
			case LauncherWorkflowPageKind::Home:
				icon = LauncherIcon::Start;
				break;
			case LauncherWorkflowPageKind::Sync:
				icon = LauncherIcon::Sync;
				break;
			case LauncherWorkflowPageKind::Build:
				icon = LauncherIcon::Build;
				break;
			case LauncherWorkflowPageKind::Cook:
				icon = LauncherIcon::Cook;
				break;
			case LauncherWorkflowPageKind::Clean:
				icon = LauncherIcon::Clean;
				break;
			case LauncherWorkflowPageKind::Unknown:
				return {};
		}

		return icons.Icon(icon, QColor(LauncherUi::Color::StateQueued));
	}

	LauncherWorkflowPanel::LauncherWorkflowPanel(
	    const LauncherIconLibrary& icons,
	    const std::function<QString(const QString&)>& displayNameForOperation,
	    const std::function<void(QWidget*)>& registerFocusable,
	    const std::function<QWidget*(QWidget*)>& createOptionsPanel,
	    QWidget* parent) :
	    QWidget(parent),
	    m_workflows(CreateLauncherWorkflowCatalog())
	{
		setObjectName("WorkflowSurface");
		QHBoxLayout* surfaceLayout = new QHBoxLayout(this);
		surfaceLayout->setContentsMargins(0, 0, 0, 0);
		surfaceLayout->setSpacing(0);

		QFrame* rail = new QFrame(this);
		rail->setObjectName("ProcessPanel");
		rail->setFixedWidth(LauncherUi::Shell::RailWidth);
		QVBoxLayout* railLayout = new QVBoxLayout(rail);
		railLayout->setContentsMargins(0, LauncherUi::Shell::RailTopPadding, 0, LauncherUi::Shell::RailBottomPadding);
		railLayout->setSpacing(LauncherUi::Space::XSmall);

		QVBoxLayout* groupLayout = new QVBoxLayout();
		groupLayout->setContentsMargins(0, 0, 0, 0);
		groupLayout->setSpacing(LauncherUi::Shell::RailGroupSpacing);

		m_workflowGroupButtons = new QButtonGroup(this);
		m_workflowGroupButtons->setExclusive(true);
		m_operationButtons = new QButtonGroup(this);
		m_operationButtons->setExclusive(true);
		m_operationTabs = new QStackedWidget(this);
		m_operationTabs->setObjectName("OperationStack");

		for (int workflowIndex = 0; workflowIndex < m_workflows.size(); ++workflowIndex)
		{
			const LauncherWorkflowDefinition& workflow = m_workflows[workflowIndex];
			const QString workflowTitle = LauncherWorkflowPageKindName(workflow.PageKind);
			QToolButton* groupButton = new QToolButton(rail);
			groupButton->setText(workflowTitle);
			groupButton->setObjectName("WorkflowGroupButton");
			groupButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
			groupButton->setFixedSize(LauncherUi::Shell::RailWidth, LauncherUi::Shell::RailItemMinHeight);
			groupButton->setProperty("WorkflowIndex", workflowIndex);
			groupButton->setProperty("ActiveState", "false");
			groupButton->setAccessibleName(workflowTitle + " workflow group");
			groupButton->setIcon(WorkflowIcon(icons, workflow.PageKind));
			groupButton->setIconSize(QSize(LauncherUi::Shell::RailIconSize, LauncherUi::Shell::RailIconSize));
			registerFocusable(groupButton);
			m_workflowGroupButtons->addButton(groupButton);
			groupLayout->addWidget(groupButton);

			QWidget* tabPage = new QWidget(m_operationTabs);
			QHBoxLayout* tabLayout = new QHBoxLayout(tabPage);
			tabLayout->setContentsMargins(0, 0, 0, 0);
			tabLayout->setSpacing(LauncherUi::Shell::WorkflowTabSpacing);
			if (workflow.OperationIds.size() > 1)
			{
				for (const QString& operationId : workflow.OperationIds)
				{
					QPushButton* operationButton = new QPushButton(displayNameForOperation(operationId), tabPage);
					operationButton->setObjectName("WorkflowButton");
					operationButton->setCheckable(true);
					operationButton->setMinimumHeight(LauncherUi::Shell::TabMinHeight);
					operationButton->setProperty("OperationId", operationId);
					operationButton->setAccessibleName(operationButton->text() + " workflow");
					registerFocusable(operationButton);
					m_operationButtons->addButton(operationButton);
					tabLayout->addWidget(operationButton);
				}
			}
			tabLayout->addStretch(1);
			const int pageIndex = m_operationTabs->addWidget(tabPage);
			for (const QString& operationId : workflow.OperationIds)
			{
				m_workflowPageByOperation.insert(operationId, pageIndex);
			}
		}

		groupLayout->addStretch(1);
		railLayout->addLayout(groupLayout, 1);
		connect(
		    m_workflowGroupButtons,
		    &QButtonGroup::buttonClicked,
		    this,
		    [this](QAbstractButton* button) { SelectWorkflowGroup(button); });
		connect(
		    m_operationButtons,
		    &QButtonGroup::buttonClicked,
		    this,
		    [this](QAbstractButton* button)
		    {
			    if (button != nullptr)
			    {
				    emit OperationSelected(button->property("OperationId").toString());
			    }
		    });

		QWidget* optionsPanel = createOptionsPanel(this);
		QVBoxLayout* optionsLayout = qobject_cast<QVBoxLayout*>(optionsPanel->layout());
		Q_ASSERT(optionsLayout != nullptr);
		m_operationTabs->setParent(optionsPanel);
		optionsLayout->insertWidget(0, m_operationTabs, 0);

		surfaceLayout->addWidget(rail, 0);
		surfaceLayout->addWidget(optionsPanel, 1);
	}

	QString LauncherWorkflowPanel::InitialOperationId() const
	{
		if (m_workflows.empty() || m_workflows.front().OperationIds.empty())
		{
			return {};
		}

		return m_workflows.front().OperationIds.front();
	}

	void LauncherWorkflowPanel::SetSelectedOperation(const QString& operationId)
	{
		for (QAbstractButton* button : m_operationButtons->buttons())
		{
			button->setChecked(button->property("OperationId").toString() == operationId);
		}

		if (!m_workflowPageByOperation.contains(operationId))
		{
			return;
		}

		const int workflowIndex = m_workflowPageByOperation.value(operationId);
		m_lastOperationByWorkflowIndex.insert(workflowIndex, operationId);
		m_operationTabs->setCurrentIndex(workflowIndex);
		SetActiveWorkflowGroup(workflowIndex);
		if (workflowIndex >= 0 && workflowIndex < m_workflows.size())
		{
			m_operationTabs->setVisible(m_workflows[workflowIndex].OperationIds.size() > 1);
		}
	}

	void LauncherWorkflowPanel::SelectWorkflowGroup(QAbstractButton* button)
	{
		if (button == nullptr)
		{
			return;
		}

		const int workflowIndex = button->property("WorkflowIndex").toInt();
		if (workflowIndex < 0 || workflowIndex >= m_workflows.size())
		{
			return;
		}

		const LauncherWorkflowDefinition& workflow = m_workflows[workflowIndex];
		if (workflow.OperationIds.empty())
		{
			return;
		}

		const QString lastOperationId = m_lastOperationByWorkflowIndex.value(workflowIndex);
		emit OperationSelected(workflow.OperationIds.contains(lastOperationId) ? lastOperationId : workflow.OperationIds.front());
	}

	void LauncherWorkflowPanel::SetActiveWorkflowGroup(int workflowIndex)
	{
		for (QAbstractButton* button : m_workflowGroupButtons->buttons())
		{
			const bool active = button != nullptr && button->property("WorkflowIndex").toInt() == workflowIndex;
			button->setProperty("ActiveState", active ? "true" : "false");
			button->style()->unpolish(button);
			button->style()->polish(button);
			button->update();
		}
	}
}
