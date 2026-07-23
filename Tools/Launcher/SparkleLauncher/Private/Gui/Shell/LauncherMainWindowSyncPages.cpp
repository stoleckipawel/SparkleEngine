#include "LauncherMainWindow.h"

#include "LauncherActionWidgets.h"
#include "LauncherArtworkWidgets.h"
#include "LauncherCleanUiModel.h"
#include "LauncherDependencyUiModel.h"
#include "LauncherHomeWidgets.h"
#include "LauncherLayoutWidgets.h"
#include "LauncherOperationRequestFactory.h"
#include "LauncherOutputWidgets.h"
#include "LauncherPageUtilities.h"
#include "LauncherProjectModel.h"
#include "LauncherSettings.h"
#include "LauncherToolchainUiModel.h"
#include "LauncherUiDesign.h"
#include "LauncherUiModel.h"
#include "LauncherWorkflowCatalog.h"

#include "SparkleLauncher/BuildWorkspaceOperations.h"
#include "SparkleLauncher/CookOperations.h"
#include "SparkleLauncher/LauncherPaths.h"
#include "SparkleLauncher/LaunchOperations.h"
#include "SparkleLauncher/MaintenanceOperations.h"
#include "SparkleLauncher/SourceDependencyState.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtGui/QColor>
#include <QtGui/QGuiApplication>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QSizePolicy>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QToolButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include <cstdint>
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <vector>

namespace SparkleLauncher
{

		enum class LevelCatalogSection
		{
			None,
			Level,
			OptionalPack
		};

		struct LauncherLevelCatalogEntry
		{
			QString Id;
			QString DisplayName;
			std::filesystem::path SourcePath;
			QString OptionalPackId;
			bool DefaultIncluded = false;
			bool Required = false;
			bool StartupDefault = false;
		};

		struct LauncherOptionalPackEntry
		{
			QString Id;
			QString DisplayName;
			std::filesystem::path RootPath;
			bool Available = true;
			bool External = false;
		};

		struct LauncherLevelCatalog
		{
			bool Loaded = false;
			QVector<LauncherLevelCatalogEntry> Levels;
			QHash<QString, LauncherOptionalPackEntry> OptionalPacks;
		};

		struct LauncherStartupLevelOption
		{
			QString DisplayName;
			QString Id;
			bool Synced = false;
			bool Ready = false;
			bool StartupDefault = false;
		};

		QString TrimCatalogValue(std::string_view value)
		{
			QString text = QString::fromStdString(std::string(value)).trimmed();
			if (text.size() >= 2 && ((text.front() == '"' && text.back() == '"') || (text.front() == '\'' && text.back() == '\'')))
			{
				text = text.mid(1, text.size() - 2);
			}
			return text;
		}

		bool TryParseCatalogBool(std::string_view value, bool& outValue)
		{
			const QString normalized = TrimCatalogValue(value).toLower();
			if (normalized == "true" || normalized == "1" || normalized == "yes" || normalized == "on")
			{
				outValue = true;
				return true;
			}
			if (normalized == "false" || normalized == "0" || normalized == "no" || normalized == "off")
			{
				outValue = false;
				return true;
			}
			return false;
		}

		bool TrySplitCatalogKeyValue(const std::string& line, std::string_view& outKey, std::string_view& outValue)
		{
			const std::size_t separator = line.find('=');
			if (separator == std::string::npos)
			{
				return false;
			}
			outKey = std::string_view(line).substr(0, separator);
			outValue = std::string_view(line).substr(separator + 1);
			while (!outKey.empty() && std::isspace(static_cast<unsigned char>(outKey.front())))
			{
				outKey.remove_prefix(1);
			}
			while (!outKey.empty() && std::isspace(static_cast<unsigned char>(outKey.back())))
			{
				outKey.remove_suffix(1);
			}
			while (!outValue.empty() && std::isspace(static_cast<unsigned char>(outValue.front())))
			{
				outValue.remove_prefix(1);
			}
			while (!outValue.empty() && std::isspace(static_cast<unsigned char>(outValue.back())))
			{
				outValue.remove_suffix(1);
			}
			return !outKey.empty();
		}

		bool WriteCatalogLines(const std::filesystem::path& catalogPath, const std::vector<std::string>& lines)
		{
			std::ofstream output(catalogPath, std::ios::binary | std::ios::trunc);
			if (!output.is_open())
			{
				return false;
			}

			for (const std::string& line : lines)
			{
				output << line << '\n';
			}
			return true;
		}

		std::string TrimCatalogLine(std::string line)
		{
			line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char character) {
				return !std::isspace(character);
			}));
			line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char character) {
				return !std::isspace(character);
			}).base(), line.end());
			return line;
		}

		bool SetCatalogSectionBool(
		    const std::filesystem::path& projectRoot,
		    std::string_view sectionHeader,
		    const QString& id,
		    std::string_view keyName,
		    bool enabled)
		{
			const std::filesystem::path catalogPath = projectRoot / "Levels.catalog";
			std::ifstream input(catalogPath, std::ios::binary);
			if (!input.is_open() || id.trimmed().isEmpty())
			{
				return false;
			}

			std::vector<std::string> lines;
			for (std::string line; std::getline(input, line);)
			{
				if (!line.empty() && line.back() == '\r')
				{
					line.pop_back();
				}
				lines.push_back(std::move(line));
			}
			input.close();

			const QString targetId = id.trimmed();
			const std::string sectionText(sectionHeader);
			const std::string keyText(keyName);
			const std::string valueLine = keyText + " = " + (enabled ? "true" : "false");
			bool inRequestedSection = false;
			bool inTargetEntry = false;
			bool targetFound = false;
			for (std::size_t lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
			{
				const std::string trimmed = TrimCatalogLine(lines[lineIndex]);
				if (trimmed.empty() || trimmed.front() == '#' || trimmed.front() == ';')
				{
					continue;
				}

				if (trimmed.front() == '[' && trimmed.back() == ']')
				{
					if (inTargetEntry)
					{
						lines.insert(lines.begin() + static_cast<std::ptrdiff_t>(lineIndex), valueLine);
						return WriteCatalogLines(catalogPath, lines);
					}
					inRequestedSection = trimmed == sectionText;
					inTargetEntry = false;
					continue;
				}

				if (!inRequestedSection)
				{
					continue;
				}

				std::string_view key;
				std::string_view value;
				if (!TrySplitCatalogKeyValue(trimmed, key, value))
				{
					continue;
				}

				if (key == "Id")
				{
					inTargetEntry = TrimCatalogValue(value) == targetId;
					targetFound = inTargetEntry;
					continue;
				}

				if (inTargetEntry && key == keyName)
				{
					lines[lineIndex] = valueLine;
					return WriteCatalogLines(catalogPath, lines);
				}
			}

			if (inTargetEntry)
			{
				lines.push_back(valueLine);
				return WriteCatalogLines(catalogPath, lines);
			}
			return targetFound;
		}

		bool SetLevelDefaultIncluded(const std::filesystem::path& projectRoot, const QString& levelId, bool enabled)
		{
			return SetCatalogSectionBool(projectRoot, "[Level]", levelId, "Default", enabled);
		}

		bool SetOptionalPackAvailable(const std::filesystem::path& projectRoot, const QString& packId, bool enabled)
		{
			return SetCatalogSectionBool(projectRoot, "[OptionalPack]", packId, "Available", enabled);
		}

		std::filesystem::path ResolveCatalogPath(const std::filesystem::path& projectRoot, std::string_view value)
		{
			std::filesystem::path path{TrimCatalogValue(value).toStdString()};
			if (path.is_relative())
			{
				path = projectRoot / path;
			}
			return path.lexically_normal();
		}

		bool CatalogPathExists(const std::filesystem::path& path)
		{
			std::error_code errorCode;
			return std::filesystem::exists(path, errorCode) && !errorCode;
		}

		QString DisplayNameOrId(const QString& displayName, const QString& id)
		{
			return displayName.trimmed().isEmpty() ? id : displayName;
		}

		const LauncherProjectSummary* FindActiveProject(const LauncherProjectModel& projectModel)
		{
			const QString activeProjectId = projectModel.ActiveProjectId();
			for (const LauncherProjectSummary& project : projectModel.Projects())
			{
				if (project.Id == activeProjectId)
				{
					return &project;
				}
			}
			return nullptr;
		}

		LauncherLevelCatalog LoadLauncherLevelCatalog(const std::filesystem::path& projectRoot)
		{
			LauncherLevelCatalog catalog;
			std::ifstream input(projectRoot / "Levels.catalog");
			if (!input.is_open())
			{
				return catalog;
			}

			catalog.Loaded = true;
			LevelCatalogSection section = LevelCatalogSection::None;
			LauncherLevelCatalogEntry* currentLevel = nullptr;
			QString currentOptionalPackId;
			for (std::string line; std::getline(input, line);)
			{
				line = TrimCatalogLine(std::move(line));
				if (line.empty() || line.front() == '#' || line.front() == ';')
				{
					continue;
				}

				if (line == "[Level]")
				{
					currentOptionalPackId.clear();
					section = LevelCatalogSection::Level;
					currentLevel = &catalog.Levels.emplace_back();
					continue;
				}
				if (line == "[OptionalPack]")
				{
					currentLevel = nullptr;
					currentOptionalPackId.clear();
					section = LevelCatalogSection::OptionalPack;
					continue;
				}

				std::string_view key;
				std::string_view value;
				if (!TrySplitCatalogKeyValue(line, key, value))
				{
					continue;
				}

				if (section == LevelCatalogSection::Level && currentLevel != nullptr)
				{
					if (key == "Id")
					{
						currentLevel->Id = TrimCatalogValue(value);
					}
					else if (key == "DisplayName")
					{
						currentLevel->DisplayName = TrimCatalogValue(value);
					}
					else if (key == "Source")
					{
						currentLevel->SourcePath = ResolveCatalogPath(projectRoot, value);
					}
					else if (key == "OptionalPack")
					{
						currentLevel->OptionalPackId = TrimCatalogValue(value);
					}
					else if (key == "Default")
					{
						TryParseCatalogBool(value, currentLevel->DefaultIncluded);
					}
					else if (key == "Required")
					{
						TryParseCatalogBool(value, currentLevel->Required);
					}
					else if (key == "StartupDefault")
					{
						TryParseCatalogBool(value, currentLevel->StartupDefault);
					}
					continue;
				}

				if (section == LevelCatalogSection::OptionalPack)
				{
					if (key == "Id")
					{
						currentOptionalPackId = TrimCatalogValue(value);
						LauncherOptionalPackEntry entry;
						entry.Id = currentOptionalPackId;
						catalog.OptionalPacks.insert(currentOptionalPackId, entry);
					}
					else if (!currentOptionalPackId.isEmpty())
					{
						LauncherOptionalPackEntry& pack = catalog.OptionalPacks[currentOptionalPackId];
						if (key == "DisplayName")
						{
							pack.DisplayName = TrimCatalogValue(value);
						}
						else if (key == "Root" || key == "Path")
						{
							pack.RootPath = ResolveCatalogPath(projectRoot, value);
						}
						else if (key == "Available")
						{
							TryParseCatalogBool(value, pack.Available);
						}
						else if (key == "External")
						{
							TryParseCatalogBool(value, pack.External);
						}
					}
				}
			}

			return catalog;
		}

		bool OptionalPackReady(const LauncherLevelCatalog& catalog, const LauncherLevelCatalogEntry& level)
		{
			if (level.OptionalPackId.isEmpty())
			{
				return true;
			}

			const auto packIt = catalog.OptionalPacks.find(level.OptionalPackId);
			if (packIt == catalog.OptionalPacks.end() || !packIt->Available)
			{
				return false;
			}

			return packIt->RootPath.empty() || CatalogPathExists(packIt->RootPath);
		}

		bool LevelReady(const LauncherLevelCatalog& catalog, const LauncherLevelCatalogEntry& level)
		{
			return !level.Id.isEmpty() && !level.SourcePath.empty() && CatalogPathExists(level.SourcePath) && OptionalPackReady(catalog, level);
		}

		bool LevelSynced(const LauncherLevelCatalogEntry& level)
		{
			return level.Required || level.DefaultIncluded;
		}

		QVector<LauncherStartupLevelOption> BuildStartupLevelEntries(const LauncherProjectSummary* activeProject)
		{
			QVector<LauncherStartupLevelOption> options;
			if (activeProject == nullptr)
			{
				return options;
			}

			const LauncherLevelCatalog catalog = LoadLauncherLevelCatalog(activeProject->RootPath);
			if (!catalog.Loaded)
			{
				return options;
			}

			const auto addLevel = [&catalog, &options](const LauncherLevelCatalogEntry& level) {
				options.push_back(LauncherStartupLevelOption{
				    .DisplayName = DisplayNameOrId(level.DisplayName, level.Id),
				    .Id = level.Id,
				    .Synced = LevelSynced(level),
				    .Ready = LevelReady(catalog, level),
				    .StartupDefault = level.StartupDefault});
			};
			for (const LauncherLevelCatalogEntry& level : catalog.Levels)
			{
				if (level.StartupDefault)
				{
					addLevel(level);
				}
			}
			for (const LauncherLevelCatalogEntry& level : catalog.Levels)
			{
				if (!level.StartupDefault)
				{
					addLevel(level);
				}
			}
			return options;
		}

		QString RelativeProjectPath(const std::filesystem::path& projectRoot, const std::filesystem::path& path)
		{
			if (path.empty())
			{
				return QString();
			}

			std::error_code errorCode;
			const std::filesystem::path relative = std::filesystem::relative(path, projectRoot, errorCode);
			return QString::fromStdString((errorCode ? path : relative).generic_string());
		}

		QString FormatBundleDetail(const DependencyGroupUiEntry& group, int readyCount)
		{
			if (!group.Enabled)
			{
				return QStringLiteral("Optional. Off in this workspace.")
				    .arg(group.ConfigureOption);
			}

			QString detail = group.Required ? QStringLiteral("Required.") : QStringLiteral("Optional and enabled.");
			detail += QStringLiteral(" %1 of %2 packages cached.")
			              .arg(readyCount)
			              .arg(group.Dependencies.size());
			return detail;
		}



	void LauncherMainWindow::AddSyncDependencyBundles(QVBoxLayout& layout)
	{
		const std::filesystem::path dependencyCachePath = GetBuildDirectory(m_repositoryRoot) / "_deps";
		QVBoxLayout* bundlesLayout = AddOptionGroup(
		    layout,
		    "Repository dependency groups",
		    QString());
		for (const DependencyGroupUiEntry& group : GetDependencyGroups())
		{
			const int readyCount = CountReadyDependencies(group, dependencyCachePath);
			AddStatusRow(
			    *bundlesLayout,
			    group.Label,
			    DependencyGroupStatusText(group, readyCount),
			    FormatBundleDetail(group, readyCount),
			    DependencyGroupStatusState(group, readyCount),
			    group.Enabled ? CreateActionDependencyActions("workspace.sync-source-tiers", "Prepare Workspace", "deps", "Clean Source Dependency Cache") :
			                    CreateDisabledSourceTierActions(group));
		}
	}

	void LauncherMainWindow::AddSyncLevelContentGroups(QVBoxLayout& layout)
	{
		QVBoxLayout* levelLayout = AddOptionGroup(
		    layout,
		    "Project level sync groups",
		    "Selectable project levels and the content roots they require.");
		const LauncherProjectSummary* activeProject = FindActiveProject(m_projectModel);
		if (activeProject == nullptr)
		{
			AddNoOptionsMessage(*levelLayout, "No active project was discovered.");
			return;
		}

		const LauncherLevelCatalog catalog = LoadLauncherLevelCatalog(activeProject->RootPath);
		if (!catalog.Loaded)
		{
			AddNoOptionsMessage(*levelLayout, "The active project has no Levels.catalog.");
			return;
		}

		for (const LauncherLevelCatalogEntry& level : catalog.Levels)
		{
			const bool ready = LevelReady(catalog, level);
			const bool synced = LevelSynced(level);
			QStringList traits;
			if (level.Required)
			{
				traits.push_back("required");
			}
			if (synced)
			{
				traits.push_back("synced");
			}
			if (level.StartupDefault)
			{
				traits.push_back("startup default");
			}
			if (!level.OptionalPackId.isEmpty())
			{
				traits.push_back(QStringLiteral("pack %1").arg(level.OptionalPackId));
			}
			const QString detail = QStringLiteral("%1%2")
			                           .arg(RelativeProjectPath(activeProject->RootPath, level.SourcePath))
			                           .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", ")));
			QCheckBox* syncBox = new QCheckBox(level.Required ? QStringLiteral("Required") : QStringLiteral("Sync"), this);
			syncBox->setChecked(synced);
			syncBox->setEnabled(!level.Required);
			RegisterFocusable(syncBox);
			connect(syncBox, &QCheckBox::toggled, this, [this, projectRoot = activeProject->RootPath, levelId = level.Id, syncBox](bool checked) {
				if (!SetLevelDefaultIncluded(projectRoot, levelId, checked))
				{
					const QSignalBlocker blocker(syncBox);
					syncBox->setChecked(!checked);
					return;
				}
				ScheduleUiRefresh(false);
			});
			AddStatusRow(
			    *levelLayout,
			    DisplayNameOrId(level.DisplayName, level.Id),
			    !synced ? "Off" : ready ? (level.Required ? "Required" : "Synced") : "Missing",
			    detail,
			    !synced ? "neutral" : ready ? "ok" : (level.Required ? "bad" : "warning"),
			    syncBox);
		}

		if (catalog.OptionalPacks.empty())
		{
			return;
		}

		QVBoxLayout* packLayout = AddDetailsGroup(
		    layout,
		    "Optional content packs",
		    "External or optional content roots referenced by level sync groups.",
		    true);
		for (const LauncherOptionalPackEntry& pack : catalog.OptionalPacks)
		{
			const bool rootReady = pack.RootPath.empty() || CatalogPathExists(pack.RootPath);
			const bool ready = pack.Available && rootReady;
			QStringList traits;
			if (pack.External)
			{
				traits.push_back("external");
			}
			if (!pack.Available)
			{
				traits.push_back("not synced");
			}
			QCheckBox* syncBox = new QCheckBox(QStringLiteral("Sync"), this);
			syncBox->setChecked(pack.Available);
			RegisterFocusable(syncBox);
			connect(syncBox, &QCheckBox::toggled, this, [this, projectRoot = activeProject->RootPath, packId = pack.Id, syncBox](bool checked) {
				if (!SetOptionalPackAvailable(projectRoot, packId, checked))
				{
					const QSignalBlocker blocker(syncBox);
					syncBox->setChecked(!checked);
					return;
				}
				ScheduleUiRefresh(false);
			});
			AddStatusRow(
			    *packLayout,
			    DisplayNameOrId(pack.DisplayName, pack.Id),
			    !pack.Available ? "Off" : ready ? "Present" : (pack.External ? "External" : "Missing"),
			    QStringLiteral("%1%2")
			        .arg(RelativeProjectPath(activeProject->RootPath, pack.RootPath))
			        .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", "))),
			    !pack.Available ? "neutral" : ready ? "ok" : "warning",
			    syncBox);
		}
	}

	QComboBox* LauncherMainWindow::CreateStartupLevelCombo()
	{
		QComboBox* combo = new QComboBox(this);
		combo->setProperty("StartupLevelSelector", true);
		combo->setToolTip("Startup level used by editor and runtime launches.");
		combo->setAccessibleName("Startup level");
		combo->setAccessibleDescription("Startup level used by editor and runtime launches.");
		RegisterFocusable(combo);
		m_startupLevelSelectors.push_back(combo);
		PopulateStartupLevelCombo(*combo);
		connect(combo, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [combo, this]() {
			const QString levelId = combo->currentData().toString();
			if (!levelId.isEmpty())
			{
				m_settings.SetLaunchStartupLevel(levelId);
			}
		});
		return combo;
	}

	void LauncherMainWindow::PopulateStartupLevelCombo(QComboBox& combo)
	{
		const QSignalBlocker blocker(&combo);
		combo.clear();
		const QVector<LauncherStartupLevelOption> options = BuildStartupLevelEntries(FindActiveProject(m_projectModel));
		if (options.empty())
		{
			combo.addItem(QStringLiteral("No catalog levels"), QString());
			combo.setEnabled(false);
			combo.setToolTip("No catalog levels are available for the active project.");
			return;
		}

		combo.setEnabled(true);
		combo.setToolTip("Startup level used by editor and runtime launches.");
		const QIcon syncedIcon = m_icons.Icon(LauncherIcon::Done, QColor(LauncherUi::Color::StateSuccess));
		const QIcon missingIcon = m_icons.Icon(LauncherIcon::Failed, QColor(LauncherUi::Color::StateWarning));
		const QIcon unsyncedIcon = m_icons.Icon(LauncherIcon::Sync, QColor(LauncherUi::Color::StateQueued));
		int selectedIndex = -1;
		int startupDefaultIndex = -1;
		int firstSelectableIndex = -1;
		for (const LauncherStartupLevelOption& option : options)
		{
			const bool selectable = option.Synced && option.Ready;
			const QString status = !option.Synced ? "not synced" : option.Ready ? "synced" : "missing";
			const QIcon icon = selectable ? syncedIcon : option.Synced ? missingIcon : unsyncedIcon;
			combo.addItem(icon, option.DisplayName, option.Id);
			const int row = combo.count() - 1;
			combo.setItemData(row, QStringLiteral("%1: %2").arg(option.DisplayName, status), Qt::ToolTipRole);
			if (QStandardItemModel* model = qobject_cast<QStandardItemModel*>(combo.model()))
			{
				if (QStandardItem* item = model->item(row))
				{
					item->setEnabled(selectable);
				}
			}
			if (selectable && firstSelectableIndex < 0)
			{
				firstSelectableIndex = row;
			}
			if (selectable && option.StartupDefault)
			{
				startupDefaultIndex = row;
			}
			if (selectable && option.Id == m_settings.LaunchStartupLevel())
			{
				selectedIndex = row;
			}
		}

		if (selectedIndex < 0)
		{
			selectedIndex = startupDefaultIndex >= 0 ? startupDefaultIndex : firstSelectableIndex;
		}
		const bool hasSelectableLevel = selectedIndex >= 0;
		combo.setEnabled(hasSelectableLevel);
		if (!hasSelectableLevel)
		{
			combo.setToolTip("No synced startup level is ready for editor or runtime launch.");
		}
		combo.setCurrentIndex(hasSelectableLevel ? selectedIndex : 0);
		const QString effectiveLevelId = combo.currentData().toString();
		if (hasSelectableLevel && !effectiveLevelId.isEmpty() && m_settings.LaunchStartupLevel() != effectiveLevelId)
		{
			m_settings.SetLaunchStartupLevel(effectiveLevelId);
		}
	}

	void LauncherMainWindow::PopulateStartupLevelSelectors()
	{
		for (QComboBox* combo : m_startupLevelSelectors)
		{
			if (combo != nullptr)
			{
				PopulateStartupLevelCombo(*combo);
			}
		}
	}

	QVector<QPair<QString, QString>> LauncherMainWindow::BuildStartupLevelOptions() const
	{
		QVector<QPair<QString, QString>> options;
		for (const LauncherStartupLevelOption& option : BuildStartupLevelEntries(FindActiveProject(m_projectModel)))
		{
			if (option.Synced && option.Ready)
			{
				options.push_back({option.DisplayName, option.Id});
			}
		}
		return options;
	}

	QString LauncherMainWindow::ResolveStartupLevelDisplayName() const
	{
		const QVector<QPair<QString, QString>> options = BuildStartupLevelOptions();
		const QString selectedLevel = m_settings.LaunchStartupLevel();
		for (const QPair<QString, QString>& option : options)
		{
			if (!selectedLevel.isEmpty() && option.second == selectedLevel)
			{
				return option.first;
			}
		}
		if (!selectedLevel.isEmpty())
		{
			return selectedLevel;
		}
		return options.empty() ? QStringLiteral("project default") : options.front().first;
	}
}
