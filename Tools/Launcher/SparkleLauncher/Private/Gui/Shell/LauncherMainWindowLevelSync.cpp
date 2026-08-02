#include "LauncherMainWindow.h"

#include "LauncherLevelUiModel.h"
#include "LauncherProjectModel.h"
#include "LauncherUiDesign.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <QtCore/QSet>
#include <QtCore/QSignalBlocker>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <string>

namespace SparkleLauncher
{
	void LauncherMainWindow::AddSyncLevelContentGroups(QVBoxLayout& layout)
	{
		QVBoxLayout* levelLayout =
		    AddOptionGroup(layout, "Project level sync groups", "Selectable project levels and the content roots they require.");
		const LauncherProjectSummary* activeProject = m_projectModel.ActiveProject();
		if (activeProject == nullptr)
		{
			AddNoOptionsMessage(*levelLayout, "No active project was discovered.");
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*activeProject);
		if (!model.Loaded)
		{
			AddNoOptionsMessage(*levelLayout, "The active project has no Levels.catalog.");
			return;
		}

		AddSyncLevelRows(*levelLayout, *activeProject, model);
		AddSyncContentPackRows(layout, *activeProject, model);
	}

	void LauncherMainWindow::AddSyncLevelRows(QVBoxLayout& layout, const LauncherProjectSummary& project, const LauncherLevelUiModel& model)
	{
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Family.isEmpty())
			{
				AddSyncLevelRow(layout, project, level);
			}
		}

		QSet<QString> renderedFamilies;
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Family.isEmpty() || renderedFamilies.contains(level.Family))
			{
				continue;
			}
			renderedFamilies.insert(level.Family);
			QVBoxLayout* familyLayout = AddDetailsGroup(
			    layout,
			    level.Family + " variants",
			    "Variants share one scene family and are not independent content products.",
			    false);
			for (const LauncherLevelUiEntry& familyLevel : model.Levels)
			{
				if (familyLevel.Family == level.Family)
				{
					AddSyncLevelRow(*familyLayout, project, familyLevel);
				}
			}
		}
	}

	void LauncherMainWindow::AddSyncLevelRow(QVBoxLayout& layout, const LauncherProjectSummary& project, const LauncherLevelUiEntry& level)
	{
		QCheckBox* syncBox = new QCheckBox(QStringLiteral("Sync"), this);
		syncBox->setChecked(level.Synced);
		syncBox->setEnabled(level.Selectable);
		if (!level.Selectable)
		{
			syncBox->setToolTip(level.UnsupportedReason);
		}
		RegisterFocusable(syncBox);

		connect(
		    syncBox,
		    &QCheckBox::toggled,
		    this,
		    [this, projectRoot = project.RootPath, levelId = level.Id, syncBox](bool checked)
		    {
			    std::string errorMessage;
			    if (!ProjectLevelCatalogFile::SetLevelDefaultIncluded(projectRoot, levelId.toStdString(), checked, errorMessage))
			    {
				    const QSignalBlocker blocker(syncBox);
				    syncBox->setChecked(!checked);
				    return;
			    }

			    ScheduleUiRefresh(false);
		    });

		AddStatusRow(layout, level.DisplayName, level.Status, level.Detail, level.State, syncBox);
	}

	void LauncherMainWindow::AddSyncContentPackRows(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherLevelUiModel& model)
	{
		if (model.ContentPacks.empty())
		{
			return;
		}

		QVBoxLayout* packLayout =
		    AddDetailsGroup(layout, "Optional content packs", "External or optional content roots referenced by level sync groups.", true);
		for (const LauncherContentPackUiEntry& pack : model.ContentPacks)
		{
			if (pack.ParentPackId.isEmpty())
			{
				AddSyncContentPackRow(*packLayout, project, pack);
			}
		}

		for (const LauncherContentPackUiEntry& parentPack : model.ContentPacks)
		{
			bool hasAddOns = false;
			for (const LauncherContentPackUiEntry& pack : model.ContentPacks)
			{
				hasAddOns = hasAddOns || pack.ParentPackId == parentPack.Id;
			}
			if (!hasAddOns)
			{
				continue;
			}

			QVBoxLayout* addOnLayout = AddDetailsGroup(
			    *packLayout,
			    parentPack.DisplayName + " add-ons",
			    "Optional layers for the parent scene. Selecting a supported add-on also acquires its base scene.",
			    true);
			for (const LauncherContentPackUiEntry& pack : model.ContentPacks)
			{
				if (pack.ParentPackId == parentPack.Id)
				{
					AddSyncContentPackRow(*addOnLayout, project, pack);
				}
			}
		}
	}

	void LauncherMainWindow::AddSyncContentPackRow(
	    QVBoxLayout& layout,
	    const LauncherProjectSummary& project,
	    const LauncherContentPackUiEntry& pack)
	{
		QCheckBox* syncBox = new QCheckBox(QStringLiteral("Download"), this);
		syncBox->setChecked(pack.Selected);
		syncBox->setEnabled(pack.DownloadSupported);
		if (!pack.DownloadSupported)
		{
			syncBox->setToolTip(pack.DownloadBlocker);
		}
		RegisterFocusable(syncBox);

		connect(
		    syncBox,
		    &QCheckBox::toggled,
		    this,
		    [this, projectRoot = project.RootPath, packId = pack.Id, parentPackId = pack.ParentPackId, syncBox](bool checked)
		    {
			    std::string errorMessage;
			    if (checked && !parentPackId.isEmpty()
			        && !ProjectLevelCatalogFile::SetOptionalContentPackAvailable(
			            projectRoot,
			            parentPackId.toStdString(),
			            true,
			            errorMessage))
			    {
				    const QSignalBlocker blocker(syncBox);
				    syncBox->setChecked(false);
				    return;
			    }
			    if (!ProjectLevelCatalogFile::SetOptionalContentPackAvailable(projectRoot, packId.toStdString(), checked, errorMessage))
			    {
				    const QSignalBlocker blocker(syncBox);
				    syncBox->setChecked(!checked);
				    return;
			    }

			    ScheduleUiRefresh(false);
		    });

		QWidget* actions = new QWidget(this);
		QHBoxLayout* actionsLayout = new QHBoxLayout(actions);
		actionsLayout->setContentsMargins(0, 0, 0, 0);
		actionsLayout->setSpacing(LauncherUi::Option::GroupSpacing);
		actionsLayout->addWidget(syncBox);
		if (!pack.SourcePageUrl.isEmpty())
		{
			QPushButton* sourceButton = new QPushButton(QStringLiteral("Source"), actions);
			sourceButton->setToolTip(QStringLiteral("Open the publisher's download and preview page."));
			RegisterFocusable(sourceButton);
			connect(
			    sourceButton,
			    &QPushButton::clicked,
			    this,
			    [sourcePageUrl = pack.SourcePageUrl]() { QDesktopServices::openUrl(QUrl(sourcePageUrl)); });
			actionsLayout->addWidget(sourceButton);
		}

		AddStatusRow(layout, pack.DisplayName, pack.Status, pack.Detail, pack.State, actions);
	}
}
