#pragma once

#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtGui/QIcon>
#include <QtWidgets/QFrame>

#include <cstdint>
#include <functional>

class QLabel;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QTextEdit;
class QWidget;

namespace SparkleLauncher
{
	class LauncherIconLibrary;

	class LauncherActivityPanel final : public QFrame
	{
	public:
		LauncherActivityPanel(
		    const LauncherIconLibrary& icons,
		    const std::function<void(QWidget*)>& registerFocusable,
		    QWidget* parent = nullptr);

		void ShowMessage(const QString& message);
		void RegisterRun(const QString& runId, const QString& title);
		void DisplayOperationStarted(const QString& runId, const QString& title);
		void AppendOperationOutput(const QString& runId, const QString& outputText);
		QString DisplayOperationFinished(
		    const QString& runId,
		    const QString& title,
		    const QString& statusText,
		    int exitCode,
		    const QString& recoveryHint);
		void DisplayBlockedOperation(const QString& runId, const QString& title, const QString& message);
		void AppendRunOutput(const QString& runId, const QString& text);
		void ShowRunOutput(const QString& runId);

	private:
		enum class RunState : std::uint8_t
		{
			Queued,
			Running,
			Done,
			Failed,
		};

		struct RunWidgets
		{
			QWidget* Root = nullptr;
			QFrame* Indicator = nullptr;
			QLabel* TitleLabel = nullptr;
			QLabel* StateLabel = nullptr;
		};

		struct RunRecord
		{
			QListWidgetItem* Item = nullptr;
			RunWidgets Widgets;
			RunState State = RunState::Queued;
			QString Title;
			QString Output;
		};

		RunWidgets CreateRunWidgets(const QString& title);
		QIcon IconForState(RunState state) const;
		void DisplaySelectedRunOutput(QListWidgetItem* currentItem);
		void CopySelectedRunOutput();
		void ToggleExpanded();
		void SetRunState(const QString& runId, RunState state, const QString& title);
		void SetExpanded(bool expanded);
		void UpdateRunSelectionVisuals();

		QFrame* m_detailsPanel = nullptr;
		QListWidget* m_runList = nullptr;
		QLabel* m_selectedRunSummary = nullptr;
		QTextEdit* m_operationOutput = nullptr;
		QPushButton* m_toggleOutputButton = nullptr;
		QPushButton* m_copyOutputButton = nullptr;
		QHash<QString, RunRecord> m_runs;
		QIcon m_queuedIcon;
		QIcon m_runningIcon;
		QIcon m_doneIcon;
		QIcon m_failedIcon;
		QString m_activeRunId;
		bool m_expanded = false;
	};
}
