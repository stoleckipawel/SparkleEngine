#include "LauncherMainWindow.h"

#include "LauncherArtworkWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherContentModel.h"
#include "LauncherUiDesign.h"

#include "Core/Public/Projects/ProjectLevelCatalog.h"
#include "SparkleLauncher/LauncherPaths.h"

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
	static QString LevelSyncKey(const QString& projectId, const QString& levelId)
	{
		return projectId + QChar(0x1f) + levelId;
	}

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
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			AddNoOptionsMessage(*catalogLayout, "Repository content is unavailable.");
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*content);
		if (!model.Loaded)
		{
			AddNoOptionsMessage(
			    *catalogLayout,
			    model.LoadError.isEmpty() ? QStringLiteral("The repository has no readable Levels.catalog.") : model.LoadError);
			return;
		}

		AddSyncLevelRows(*catalogLayout, *content, model);
	}

	void LauncherMainWindow::AddSyncLevelRows(QVBoxLayout& layout, const LauncherContentSummary& content, const LauncherLevelUiModel& model)
	{
		ResponsiveCardGridWidget* grid = new ResponsiveCardGridWidget(520, 1600, 4, 14, 14, this);
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Id == "Empty")
			{
				continue;
			}
			AddSyncLevelRow(*grid, content, level);
		}
		layout.addWidget(grid);
	}

	void LauncherMainWindow::AddSyncLevelRow(
	    ResponsiveCardGridWidget& grid,
	    const LauncherContentSummary& content,
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

		QLabel* description = new QLabel(level.Description.isEmpty() ? QStringLiteral("Level") : level.Description, body);
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
		    [this, content, level, actionButton]()
		    {
			    const QString actionState = actionButton->property("ActionState").toString();
			    if (actionState == QStringLiteral("cancel"))
				    CancelLevelSync(content, level);
			    else if (actionState == QStringLiteral("clean"))
				    CleanLevel(content, level);
			    else if (actionState == QStringLiteral("sync"))
				    SyncLevel(content, level);
		    });
		actions->addWidget(actionButton);
		bodyLayout->addLayout(actions);
		cardLayout->addWidget(body, 3);
		card->SetActivationButton(actionButton);
		grid.AddCard(card);
	}

	void LauncherMainWindow::ApplyLevelActionButtonState(QPushButton& button, const LauncherLevelUiEntry& level)
	{
		const QString syncRunId = m_levelSyncRunIds.value(LevelSyncKey(m_contentModel.ContentId(), level.Id));
		if (!syncRunId.isEmpty())
		{
			const bool canceling = m_pendingLevelCancellations.contains(syncRunId);
			button.setText(canceling ? QStringLiteral("Canceling...") : QStringLiteral("Cancel"));
			button.setProperty("ActionState", canceling ? "canceling" : "cancel");
			button.setEnabled(!canceling);
			button.setAccessibleName(
			    (canceling ? QStringLiteral("Canceling sync for ") : QStringLiteral("Cancel sync for ")) + level.DisplayName);
			button.setToolTip(
			    canceling
			        ? QStringLiteral("Waiting for the acquisition process to stop before removing partial content.")
			        : QStringLiteral("Cancel this sync, remove its downloaded and partially staged content, and deselect the level."));
			button.style()->unpolish(&button);
			button.style()->polish(&button);
			return;
		}

		const bool clean = (level.Selected && level.Ready) || (!level.RuntimeSupported && level.CanClean);
		button.setText(clean ? QStringLiteral("Clean") : QStringLiteral("Sync"));
		button.setProperty("ActionState", clean ? "clean" : "sync");
		button.setEnabled(clean || level.CanSelect || level.CanSync);
		button.setAccessibleName((clean ? QStringLiteral("Clean ") : QStringLiteral("Sync ")) + level.DisplayName);
		if (clean)
		{
			button.setToolTip(
			    level.RuntimeSupported
			        ? QStringLiteral("Disable this level and clean its downloaded content.")
			        : QStringLiteral("Remove this source pack. Runtime use remains unavailable: ") + level.UnsupportedReason);
		}
		else if (level.CanSync && !level.RuntimeSupported)
		{
			button.setToolTip(QStringLiteral("Download this source pack. Runtime use remains unavailable: ") + level.UnsupportedReason);
		}
		else if (!level.CanSelect)
		{
			button.setToolTip(level.UnsupportedReason);
		}
		else
		{
			button.setToolTip(QStringLiteral("Enable this level and acquire any missing content."));
		}
		button.style()->unpolish(&button);
		button.style()->polish(&button);
	}

	void LauncherMainWindow::RefreshLevelActionButtons()
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*content);
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
	    const std::filesystem::path& contentRoot,
	    const std::vector<std::string>& levelIds,
	    bool selected,
	    const QString& actionName)
	{
		std::string errorMessage;
		if (ProjectLevelCatalogFile::SetLevelsSelected(contentRoot, levelIds, selected, errorMessage))
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
	    const LauncherContentSummary& content,
	    const QString& levelId,
	    LevelCleanMode mode) const
	{
		const ProjectLevelCatalog catalog = ProjectLevelCatalogFile::Load(content.RootPath);
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
			const auto appendExistingTarget = [&targets](std::string displayName, const std::filesystem::path& path, QString detail)
			{
				std::error_code existsError;
				if (path.empty() || !std::filesystem::exists(path, existsError) || existsError)
				{
					return;
				}

				LauncherCleanTarget target;
				target.DisplayName = QString::fromStdString(std::move(displayName));
				target.Path = QString::fromStdString(path.string());
				target.Detail = std::move(detail);
				targets.push_back(std::move(target));
			};

			if (pack.external && !pack.extractionPath.empty())
			{
				appendExistingTarget(
				    pack.displayName + " level content",
				    pack.extractionPath,
				    mode == LevelCleanMode::PurgeWithoutConfirmation
				        ? QStringLiteral("Extracted level content created by the canceled sync.")
				        : QStringLiteral("Extracted level content. The cached source archive is preserved for fast re-sync."));

				if (mode == LevelCleanMode::PurgeWithoutConfirmation)
				{
					const std::filesystem::path stagingRoot = pack.extractionPath.string() + ".sparkle-staging-" + pack.id;
					const std::filesystem::path backupRoot = pack.extractionPath.string() + ".sparkle-backup-" + pack.id;
					appendExistingTarget(
					    pack.displayName + " partial staging",
					    stagingRoot,
					    QStringLiteral("Partial extraction from the canceled sync."));
					appendExistingTarget(
					    pack.displayName + " sync backup",
					    backupRoot,
					    QStringLiteral("Temporary publication backup from the canceled sync."));

					if (pack.downloadSupported)
					{
						const std::filesystem::path cacheRoot =
						    GetLauncherStateDirectory(m_repositoryRoot) / "ContentArchives" / content.Id.toStdString();
						const std::filesystem::path archivePath = cacheRoot / pack.archiveName;
						appendExistingTarget(
						    pack.displayName + " cached archive",
						    archivePath,
						    QStringLiteral("Cached archive from the canceled sync."));
						appendExistingTarget(
						    pack.displayName + " partial archive",
						    archivePath.string() + ".partial",
						    QStringLiteral("Incomplete archive download from the canceled sync."));
					}
				}
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

	void LauncherMainWindow::SyncLevel(const LauncherContentSummary& content, const LauncherLevelUiEntry& level)
	{
		const QString syncKey = LevelSyncKey(content.Id, level.Id);
		if (m_levelSyncRunIds.contains(syncKey))
		{
			CancelLevelSync(content, level);
			return;
		}

		if (level.RuntimeSupported
		    && !SetLevelsSelected(content.RootPath, {level.Id.toStdString()}, true, QStringLiteral("Sync ") + level.DisplayName))
		{
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("workspace.sync-levels"));
		request.ContentId = content.Id;
		request.RequestedLevelIds = level.Id;
		const QString runId = StartOperation(std::move(request), QStringLiteral("Sync ") + level.DisplayName);
		m_levelSyncRunIds.insert(syncKey, runId);
		RefreshLevelActionButtons();
	}

	void LauncherMainWindow::CancelLevelSync(const LauncherContentSummary& content, const LauncherLevelUiEntry& level)
	{
		const QString runId = m_levelSyncRunIds.value(LevelSyncKey(content.Id, level.Id));
		if (runId.isEmpty() || m_pendingLevelCancellations.contains(runId))
		{
			return;
		}

		m_pendingLevelCancellations.insert(runId, {content.RootPath, content.Id, level.Id, level.DisplayName});
		AppendRunOutput(runId, QStringLiteral("\nCancel requested. Stopping sync before removing partial content.\n"));
		m_backend.CancelOperation(runId);
		RefreshLevelActionButtons();
	}

	void LauncherMainWindow::CleanCanceledLevelSync(const PendingLevelCancellation& cancellation)
	{
		LauncherContentSummary content;
		content.Id = cancellation.ContentId;
		content.DisplayName = cancellation.ContentId;
		content.RootPath = cancellation.ContentRoot;
		LauncherLevelUiEntry level;
		level.Id = cancellation.LevelId;
		level.DisplayName = cancellation.LevelDisplayName;
		CleanLevel(content, level, LevelCleanMode::PurgeWithoutConfirmation);
	}

	void LauncherMainWindow::CleanLevel(const LauncherContentSummary& content, const LauncherLevelUiEntry& level, LevelCleanMode mode)
	{
		const std::vector<std::string> levelIds{level.Id.toStdString()};
		QVector<LauncherCleanTarget> targets;
		try
		{
			targets = BuildLevelCleanTargets(content, level.Id, mode);
		}
		catch (const std::exception& error)
		{
			QMessageBox::warning(this, QStringLiteral("Level content could not be resolved"), QString::fromUtf8(error.what()));
			return;
		}
		if (targets.empty())
		{
			if (SetLevelsSelected(content.RootPath, levelIds, false, QStringLiteral("Clean ") + level.DisplayName))
			{
				PopulateStartupLevelSelectors();
				RefreshLevelActionButtons();
				UpdateRunAvailability();
			}
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("workspace.clean"));
		request.ContentId = content.Id;
		request.CleanTargets = targets;
		request.ConfirmClean = mode == LevelCleanMode::PurgeWithoutConfirmation;
		if (mode == LevelCleanMode::PreserveArchiveWithConfirmation && !ConfirmRunRequest(request))
		{
			return;
		}

		const QString title = QStringLiteral("Clean ") + level.DisplayName;
		const QString runId = StartOperation(std::move(request), title);
		m_pendingLevelSelectionUpdates.insert(runId, {content.RootPath, levelIds, false});
		if (QPushButton* button = m_levelActionButtons.value(level.Id, nullptr))
		{
			button->setText(QStringLiteral("Cleaning..."));
			button->setEnabled(false);
		}
	}

	void LauncherMainWindow::SyncAllLevels()
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*content);
		std::vector<std::string> levelIds;
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			if (level.Id != "Empty" && level.CanSelect)
			{
				levelIds.push_back(level.Id.toStdString());
			}
		}
		if (!SetLevelsSelected(content->RootPath, levelIds, true, QStringLiteral("Sync All")))
		{
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("workspace.sync-levels"));
		StartOperation(std::move(request), QStringLiteral("Sync All Levels"));
	}

	void LauncherMainWindow::CleanAllLevels()
	{
		const LauncherContentSummary* content = m_contentModel.Content();
		if (content == nullptr)
		{
			return;
		}

		const LauncherLevelUiModel model = LauncherLevelUiModel::Build(*content);
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
			targets = BuildLevelCleanTargets(*content);
		}
		catch (const std::exception& error)
		{
			QMessageBox::warning(this, QStringLiteral("Level content could not be resolved"), QString::fromUtf8(error.what()));
			return;
		}
		if (targets.empty())
		{
			if (SetLevelsSelected(content->RootPath, levelIds, false, QStringLiteral("Clean All")))
			{
				PopulateStartupLevelSelectors();
				RefreshLevelActionButtons();
				UpdateRunAvailability();
			}
			return;
		}

		LauncherOperationRequest request =
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("workspace.clean"));
		request.CleanTargets = targets;
		request.ConfirmClean = false;
		if (!ConfirmRunRequest(request))
		{
			return;
		}

		const QString runId = StartOperation(std::move(request), QStringLiteral("Clean All Levels"));
		m_pendingLevelSelectionUpdates.insert(runId, {content->RootPath, levelIds, false});
	}
}
