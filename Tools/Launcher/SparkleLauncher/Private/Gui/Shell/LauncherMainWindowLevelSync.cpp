#include "LauncherMainWindow.h"

#include "LauncherArtworkWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherProjectModel.h"
#include "LauncherUiDesign.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <QtCore/QFileInfo>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QLinearGradient>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtGui/QPixmap>
#include <QtWidgets/QFrame>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <exception>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace SparkleLauncher
{
	static QPixmap CreateMapPlaceholder(const LauncherLevelUiEntry& level)
	{
		QPixmap placeholder(640, 400);
		const uint hueSeed = qHash(level.Id);
		const QColor topColor = QColor::fromHsv(static_cast<int>(hueSeed % 80u) + 75, 125, 74);
		const QColor bottomColor = QColor::fromHsv(static_cast<int>((hueSeed + 22u) % 80u) + 75, 175, 24);
		QLinearGradient gradient(0, 0, placeholder.width(), placeholder.height());
		gradient.setColorAt(0.0, topColor);
		gradient.setColorAt(1.0, bottomColor);

		QPainter painter(&placeholder);
		painter.fillRect(placeholder.rect(), gradient);
		painter.setPen(QPen(QColor(255, 255, 255, 28), 3));
		painter.drawLine(0, placeholder.height() * 3 / 4, placeholder.width(), placeholder.height() / 3);
		painter.drawEllipse(QPoint(placeholder.width() * 4 / 5, placeholder.height() / 5), 84, 84);
		painter.setPen(QPen(QColor(232, 242, 224, 115), 5));
		const QRect marker(placeholder.width() / 2 - 54, placeholder.height() / 2 - 44, 108, 88);
		painter.drawRoundedRect(marker, 8, 8);
		painter.drawLine(marker.left() + 18, marker.bottom() - 18, marker.center().x(), marker.top() + 20);
		painter.drawLine(marker.center().x(), marker.top() + 20, marker.right() - 18, marker.bottom() - 18);
		return placeholder;
	}

	static QPixmap LoadMapArtwork(const LauncherLevelUiEntry& level)
	{
		if (!level.ThumbnailPath.isEmpty() && QFileInfo::exists(level.ThumbnailPath))
		{
			QPixmap artwork(level.ThumbnailPath);
			if (!artwork.isNull())
			{
				return artwork;
			}
		}
		return CreateMapPlaceholder(level);
	}

	void LauncherMainWindow::AddSyncLevelContentGroups(QVBoxLayout& layout)
	{
		QVBoxLayout* catalogLayout =
		    AddOptionGroup(layout, "Map catalog", "Sync or clean levels directly. Empty remains available without external content.");
		const LauncherProjectSummary* activeProject = m_projectModel.ActiveProject();
		if (activeProject == nullptr)
		{
			AddNoOptionsMessage(*catalogLayout, "No active project was discovered.");
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*activeProject);
		if (!model.Loaded)
		{
			AddNoOptionsMessage(
			    *catalogLayout,
			    model.LoadError.isEmpty() ? QStringLiteral("The active project has no readable Levels.catalog.") : model.LoadError);
			return;
		}

		AddSyncLevelRows(*catalogLayout, *activeProject, model);
	}

	void LauncherMainWindow::AddSyncLevelRows(QVBoxLayout& layout, const LauncherProjectSummary& project, const LauncherLevelUiModel& model)
	{
		ResponsiveCardGridWidget* grid = new ResponsiveCardGridWidget(520, 1600, 4, 14, 14, this);
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Id == "Empty")
			{
				continue;
			}
			AddSyncLevelRow(*grid, project, level);
		}
		layout.addWidget(grid);
	}

	void LauncherMainWindow::AddSyncLevelRow(
	    ResponsiveCardGridWidget& grid,
	    const LauncherProjectSummary& project,
	    const LauncherLevelUiEntry& level)
	{
		ProportionalCardFrame* card = new ProportionalCardFrame(2.75, this);
		card->setObjectName("MapCatalogCard");
		card->setProperty("State", level.State);
		card->setAccessibleName(level.DisplayName);
		card->setAccessibleDescription(level.Description);

		QHBoxLayout* cardLayout = new QHBoxLayout(card);
		cardLayout->setContentsMargins(0, 0, 0, 0);
		cardLayout->setSpacing(0);
		LauncherArtworkSpec artworkSpec;
		artworkSpec.BaseColor = QColor("#0b0d0c");
		artworkSpec.AspectRatio = 16.0 / 10.0;
		LauncherArtworkWidget* artwork = new LauncherArtworkWidget(LoadMapArtwork(level), artworkSpec, QSize(640, 400), card);
		artwork->setObjectName("MapCardThumbnail");
		artwork->setMinimumWidth(178);
		cardLayout->addWidget(artwork, 2);

		QWidget* body = new QWidget(card);
		body->setObjectName("MapCardBody");
		body->setAttribute(Qt::WA_StyledBackground, true);
		QVBoxLayout* bodyLayout = new QVBoxLayout(body);
		bodyLayout->setContentsMargins(11, 0, 11, 9);
		bodyLayout->setSpacing(5);

		QLabel* title = new QLabel(level.DisplayName, body);
		title->setObjectName("MapCardTitle");
		title->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		bodyLayout->addWidget(title);

		QLabel* description = new QLabel(level.Description.isEmpty() ? QStringLiteral("Project map") : level.Description, body);
		description->setObjectName("MapCardDescription");
		description->setWordWrap(true);
		description->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		bodyLayout->addWidget(description, 1);

		QLabel* detail = new QLabel(level.Detail, body);
		detail->setObjectName("MapCardMeta");
		detail->setTextInteractionFlags(Qt::TextSelectableByMouse);
		detail->setToolTip(level.Detail);
		bodyLayout->addWidget(detail);

		QHBoxLayout* actions = new QHBoxLayout();
		actions->setContentsMargins(0, 0, 0, 0);
		actions->setSpacing(8);

		if (!level.SourcePageUrl.isEmpty())
		{
			QPushButton* sourceButton = new QPushButton(QStringLiteral("Source"), body);
			sourceButton->setObjectName("MapCardSourceButton");
			sourceButton->setToolTip(QStringLiteral("Open the publisher's preview and download page."));
			RegisterFocusable(sourceButton);
			connect(
			    sourceButton,
			    &QPushButton::clicked,
			    this,
			    [sourcePageUrl = level.SourcePageUrl]() { QDesktopServices::openUrl(QUrl(sourcePageUrl)); });
			actions->addWidget(sourceButton);
		}

		actions->addStretch(1);

		QPushButton* actionButton = new QPushButton(body);
		actionButton->setObjectName("MapCardActionButton");
		ApplyLevelActionButtonState(*actionButton, level);
		m_levelActionButtons.insert(level.Id, actionButton);
		RegisterFocusable(actionButton);
		connect(
		    actionButton,
		    &QPushButton::clicked,
		    this,
		    [this, project, level, actionButton]()
		    {
			    if (actionButton->property("ActionState").toString() == QStringLiteral("clean"))
				    CleanLevel(project, level);
			    else
				    SyncLevel(project, level);
		    });
		actions->addWidget(actionButton);
		bodyLayout->addLayout(actions);
		cardLayout->addWidget(body, 3);
		card->SetActivationButton(actionButton);
		grid.AddCard(card);
	}

	void LauncherMainWindow::ApplyLevelActionButtonState(QPushButton& button, const LauncherLevelUiEntry& level)
	{
		const bool active = level.Selected && level.Ready;
		button.setText(active ? QStringLiteral("Clean") : QStringLiteral("Sync"));
		button.setProperty("ActionState", active ? "clean" : "sync");
		button.setEnabled(active || level.CanSelect);
		button.setAccessibleName((active ? QStringLiteral("Clean ") : QStringLiteral("Sync ")) + level.DisplayName);
		if (!active && !level.CanSelect)
		{
			button.setToolTip(level.UnsupportedReason);
		}
		else
		{
			button.setToolTip(
			    active ? QStringLiteral("Disable this level and clean its downloaded project content.")
			           : QStringLiteral("Enable this level and acquire any missing project content."));
		}
		button.style()->unpolish(&button);
		button.style()->polish(&button);
	}

	void LauncherMainWindow::RefreshLevelActionButtons()
	{
		const LauncherProjectSummary* project = m_projectModel.ActiveProject();
		if (project == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*project);
		if (!model.Loaded)
		{
			return;
		}

		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (QPushButton* button = m_levelActionButtons.value(level.Id, nullptr))
			{
				ApplyLevelActionButtonState(*button, level);
			}
		}
	}

	bool LauncherMainWindow::SetLevelsSelected(
	    const std::filesystem::path& projectRoot,
	    const std::vector<std::string>& levelIds,
	    bool selected,
	    const QString& actionName)
	{
		std::string errorMessage;
		if (ProjectLevelCatalogFile::SetLevelsSelected(projectRoot, levelIds, selected, errorMessage))
		{
			return true;
		}

		QMessageBox::warning(
		    this,
		    actionName + QStringLiteral(" could not update the level catalog"),
		    QString::fromStdString(errorMessage));
		return false;
	}

	QVector<LauncherCleanTarget> LauncherMainWindow::BuildLevelCleanTargets(
	    const LauncherProjectSummary& project,
	    const QString& levelId) const
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(project.RootPath);
		QVector<LauncherCleanTarget> targets;
		std::set<std::string, std::less<>> appendedPackIds;
		std::set<std::string, std::less<>> retainedPackIds;

		const auto retainPackAndParents = [&](const auto& self, std::string_view packId) -> void
		{
			if (packId.empty() || !retainedPackIds.insert(std::string(packId)).second)
			{
				return;
			}

			const auto packIt = catalog.assetPacks.find(packId);
			if (packIt != catalog.assetPacks.end())
			{
				self(self, packIt->second.parentPackId);
			}
		};

		if (!levelId.isEmpty())
		{
			for (const ProjectLevelCatalogEntry& level : catalog.levels)
			{
				if (level.selected && level.id != levelId.toStdString())
				{
					retainPackAndParents(retainPackAndParents, level.assetPackId);
				}
			}
		}

		const auto appendPackAndParents = [&](const auto& self, std::string_view packId) -> void
		{
			if (packId.empty() || retainedPackIds.contains(packId) || !appendedPackIds.insert(std::string(packId)).second)
			{
				return;
			}

			const auto packIt = catalog.assetPacks.find(packId);
			if (packIt == catalog.assetPacks.end())
			{
				return;
			}

			const ProjectAssetPack& pack = packIt->second;
			std::error_code existsError;
			if (pack.external && !pack.extractionPath.empty() && std::filesystem::exists(pack.extractionPath, existsError) && !existsError)
			{
				LauncherCleanTarget target;
				target.DisplayName = QString::fromStdString(pack.displayName + " level content");
				target.Path = QString::fromStdString(pack.extractionPath.string());
				target.Detail = QStringLiteral("Extracted level content. The cached source archive is preserved for fast re-sync.");
				targets.push_back(std::move(target));
			}

			self(self, pack.parentPackId);
		};

		if (levelId.isEmpty())
		{
			for (const ProjectLevelCatalogEntry& level : catalog.levels)
			{
				if (level.selected)
				{
					appendPackAndParents(appendPackAndParents, level.assetPackId);
				}
			}
			return targets;
		}

		for (const ProjectLevelCatalogEntry& level : catalog.levels)
		{
			if (level.id == levelId.toStdString())
			{
				appendPackAndParents(appendPackAndParents, level.assetPackId);
				break;
			}
		}
		return targets;
	}

	void LauncherMainWindow::SyncLevel(const LauncherProjectSummary& project, const LauncherLevelUiEntry& level)
	{
		if (!SetLevelsSelected(project.RootPath, {level.Id.toStdString()}, true, QStringLiteral("Sync ") + level.DisplayName))
		{
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, QStringLiteral("project.sync-levels"));
		StartOperation(std::move(request), QStringLiteral("Sync ") + level.DisplayName);
		if (QPushButton* button = m_levelActionButtons.value(level.Id, nullptr))
		{
			button->setText(QStringLiteral("Syncing..."));
			button->setEnabled(false);
		}
	}

	void LauncherMainWindow::CleanLevel(const LauncherProjectSummary& project, const LauncherLevelUiEntry& level)
	{
		const std::vector<std::string> levelIds{level.Id.toStdString()};
		QVector<LauncherCleanTarget> targets;
		try
		{
			targets = BuildLevelCleanTargets(project, level.Id);
		}
		catch (const std::exception& error)
		{
			QMessageBox::warning(this, QStringLiteral("Level content could not be resolved"), QString::fromUtf8(error.what()));
			return;
		}
		if (targets.empty())
		{
			if (SetLevelsSelected(project.RootPath, levelIds, false, QStringLiteral("Clean ") + level.DisplayName))
			{
				PopulateStartupLevelSelectors();
				RefreshLevelActionButtons();
				UpdateRunAvailability();
			}
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, QStringLiteral("workspace.clean"));
		request.CleanTargets = targets;
		request.ConfirmClean = false;
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		const QString title = QStringLiteral("Clean ") + level.DisplayName;
		const QString runId = StartOperation(std::move(request), title);
		m_pendingLevelSelectionUpdates.insert(runId, {project.RootPath, levelIds, false});
		if (QPushButton* button = m_levelActionButtons.value(level.Id, nullptr))
		{
			button->setText(QStringLiteral("Cleaning..."));
			button->setEnabled(false);
		}
	}

	void LauncherMainWindow::SyncAllLevels()
	{
		const LauncherProjectSummary* project = m_projectModel.ActiveProject();
		if (project == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*project);
		std::vector<std::string> levelIds;
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Id != "Empty" && level.CanSelect)
			{
				levelIds.push_back(level.Id.toStdString());
			}
		}
		if (!SetLevelsSelected(project->RootPath, levelIds, true, QStringLiteral("Sync All")))
		{
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, QStringLiteral("project.sync-levels"));
		StartOperation(std::move(request), QStringLiteral("Sync All Levels"));
	}

	void LauncherMainWindow::CleanAllLevels()
	{
		const LauncherProjectSummary* project = m_projectModel.ActiveProject();
		if (project == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*project);
		std::vector<std::string> levelIds;
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Id != "Empty")
			{
				levelIds.push_back(level.Id.toStdString());
			}
		}

		QVector<LauncherCleanTarget> targets;
		try
		{
			targets = BuildLevelCleanTargets(*project);
		}
		catch (const std::exception& error)
		{
			QMessageBox::warning(this, QStringLiteral("Level content could not be resolved"), QString::fromUtf8(error.what()));
			return;
		}
		if (targets.empty())
		{
			if (SetLevelsSelected(project->RootPath, levelIds, false, QStringLiteral("Clean All")))
			{
				PopulateStartupLevelSelectors();
				RefreshLevelActionButtons();
				UpdateRunAvailability();
			}
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_projectModel, m_settings, QStringLiteral("workspace.clean"));
		request.CleanTargets = targets;
		request.ConfirmClean = false;
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		const QString runId = StartOperation(std::move(request), QStringLiteral("Clean All Levels"));
		m_pendingLevelSelectionUpdates.insert(runId, {project->RootPath, levelIds, false});
	}
}
