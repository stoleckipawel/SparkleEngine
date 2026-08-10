#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherLevelUiModel.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherSettings.h"
#include "LauncherContentModel.h"
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
		if (level.Id == "Empty")
		{
			QPainter painter(&placeholder);
			painter.fillRect(placeholder.rect(), QColor("#101310"));
			painter.setPen(QPen(QColor(118, 185, 45, 45), 1));
			const QPoint horizon(placeholder.width() / 2, placeholder.height() * 2 / 5);
			for (int x = -160; x <= placeholder.width() + 160; x += 64)
			{
				painter.drawLine(horizon, QPoint(x, placeholder.height()));
			}
			for (int y = horizon.y(); y <= placeholder.height(); y += 32)
			{
				painter.drawLine(0, y, placeholder.width(), y);
			}

			painter.setPen(QPen(QColor("#76b92d"), 4));
			const QRect front(placeholder.width() / 2 - 58, placeholder.height() / 2 - 34, 116, 92);
			const QPoint offset(38, -28);
			const QRect back = front.translated(offset);
			painter.drawRect(front);
			painter.drawRect(back);
			painter.drawLine(front.topLeft(), back.topLeft());
			painter.drawLine(front.topRight(), back.topRight());
			painter.drawLine(front.bottomLeft(), back.bottomLeft());
			painter.drawLine(front.bottomRight(), back.bottomRight());
			return placeholder;
		}

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
		QVBoxLayout* catalogLayout = AddOptionGroup(
		    layout,
		    "Map catalog",
		    "Open a level using the footer Run Mode. Quick Start prepares its content, build, and cook prerequisites first.");
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
		ResponsiveCardGridWidget* grid = new ResponsiveCardGridWidget(
		    LauncherUi::MapCatalog::MinimumCardWidth,
		    LauncherUi::MapCatalog::MaximumCardWidth,
		    LauncherUi::MapCatalog::MaximumColumns,
		    LauncherUi::MapCatalog::HorizontalSpacing,
		    LauncherUi::MapCatalog::VerticalSpacing,
		    this);
		for (const LauncherLevelUiEntry& level : model.Levels)
		{
			AddSyncLevelRow(*grid, content, level);
		}
		layout.addWidget(grid);
	}

	void LauncherMainWindow::AddSyncLevelRow(
	    ResponsiveCardGridWidget& grid,
	    const LauncherContentSummary& content,
	    const LauncherLevelUiEntry& level)
	{
		ProportionalCardFrame* card = new ProportionalCardFrame(LauncherUi::MapCatalog::CardAspectRatio, this);
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

		QHBoxLayout* metadata = new QHBoxLayout();
		metadata->setContentsMargins(2, 0, 2, 0);
		metadata->setSpacing(8);
		QLabel* statusLabel = new QLabel(body);
		statusLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
		metadata->addWidget(statusLabel, 0, Qt::AlignVCenter);

		QLabel* detail = new QLabel(level.Detail, body);
		detail->setObjectName("MapCardMeta");
		detail->setTextInteractionFlags(Qt::TextSelectableByMouse);
		detail->setToolTip(level.Detail);
		metadata->addWidget(detail, 1, Qt::AlignVCenter);
		bodyLayout->addLayout(metadata);

		QHBoxLayout* actions = new QHBoxLayout();
		actions->setContentsMargins(0, 0, 0, 0);
		actions->setSpacing(8);

		if (!level.SourcePageUrl.isEmpty())
		{
			QPushButton* sourceButton = new QPushButton(QStringLiteral("Source"), body);
			sourceButton->setObjectName("MapCardSourceButton");
			sourceButton->setFixedSize(LauncherUi::Row::StatusActionWidth, LauncherUi::Row::StatusActionHeight);
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
		ApplyLevelActionButtonState(*statusLabel, *actionButton, level);
		m_levelStatusLabels.insert(level.Id, statusLabel);
		m_levelActionButtons.insert(level.Id, actionButton);
		RegisterFocusable(actionButton);
		connect(
		    actionButton,
		    &QPushButton::clicked,
		    this,
		    [this, content, level, actionButton]()
		    {
			    const QString actionIntent = actionButton->property("ActionIntent").toString();
			    if (actionIntent == QStringLiteral("run"))
			    {
				    StartQuickStartLevel(content, level);
			    }
		    });
		actions->addWidget(actionButton);
		bodyLayout->addLayout(actions);
		cardLayout->addWidget(body, 3);
		card->SetActivationButton(actionButton);
		grid.AddCard(card);
	}

	void LauncherMainWindow::ApplyLevelActionButtonState(QLabel& statusLabel, QPushButton& button, const LauncherLevelUiEntry& level)
	{
		ApplyInlineStatusLabel(statusLabel, level.Status, level.State);
		const bool preparing = m_quickStartExecution.has_value()
		    && m_quickStartExecution->GoalRequest().RequestedLevelIds.section(',', 0, 0).trimmed() == level.Id;
		button.setObjectName("MapCardActionButton");
		button.setFixedSize(LauncherUi::Row::StatusActionWidth, LauncherUi::Row::StatusActionHeight);
		button.setProperty("ActionIntent", preparing ? QStringLiteral("none") : QStringLiteral("run"));
		const bool editorMode = m_settings.RunMode() != QStringLiteral("game");
		const QString actionName = editorMode ? QStringLiteral("Open") : QStringLiteral("Run");
		button.setText(preparing ? QStringLiteral("Preparing...") : actionName);
		button.setAccessibleName(
		    preparing ? QStringLiteral("Preparing ") + level.DisplayName : actionName + QStringLiteral(" ") + level.DisplayName);
		button.setEnabled(!m_quickStartExecution.has_value() && level.RuntimeSupported && level.CanSelect);
		if (preparing)
		{
			button.setToolTip(QStringLiteral("Quick Start is preparing this level's prerequisites."));
		}
		else if (!level.RuntimeSupported || !level.CanSelect)
		{
			button.setToolTip(level.UnsupportedReason);
		}
		else
		{
			button.setToolTip(
			    editorMode ? QStringLiteral("Sync missing content, prepare prerequisites, then open this level in the editor.")
			               : QStringLiteral("Sync missing content, prepare prerequisites, then run this level in the standalone game."));
		}
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
			QLabel* statusLabel = m_levelStatusLabels.value(level.Id, nullptr);
			QPushButton* button = m_levelActionButtons.value(level.Id, nullptr);
			if (statusLabel != nullptr && button != nullptr)
			{
				ApplyLevelActionButtonState(*statusLabel, *button, level);
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
	    const QString& levelId) const
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
				    QStringLiteral("Extracted level content. The cached source archive is preserved for fast re-sync."));
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
		    BuildLauncherOperationRequest(m_repositoryRoot, m_contentModel, m_settings, QStringLiteral("levels.sync"));
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
