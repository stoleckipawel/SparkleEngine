#include "LauncherLevelUiModel.h"

#include "LauncherProjectModel.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <QtCore/QStringList>

#include <filesystem>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace SparkleLauncher
{
	class LauncherLevelUiModelBuilder final
	{
	public:
		explicit LauncherLevelUiModelBuilder(const LauncherProjectSummary& project) noexcept :
		    m_project(project)
		{
		}

		LauncherLevelUiModel Build()
		{
			try
			{
				m_catalog = ProjectLevelCatalogFile::Load(m_project.RootPath);
				m_model.Loaded = true;
			}
			catch (const Diagnostics::Error& error)
			{
				m_model.LoadError = QString::fromStdString(error.what());
				return std::move(m_model);
			}

			m_model.Levels.reserve(static_cast<qsizetype>(m_catalog.levels.size()));
			m_model.StartupLevels.reserve(static_cast<qsizetype>(m_catalog.levels.size()));
			for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
			{
				m_model.Levels.push_back(BuildLevel(level));
				if (level.selected)
				{
					m_model.StartupLevels.push_back(BuildStartupLevel(level));
				}
			}
			return std::move(m_model);
		}

	private:
		LauncherLevelUiEntry BuildLevel(const ProjectLevelCatalogEntry& level) const
		{
			const auto pack = level.assetPackId.empty() ? m_catalog.assetPacks.end() : m_catalog.assetPacks.find(level.assetPackId);
			const ProjectAssetPack* assetPack = pack == m_catalog.assetPacks.end() ? nullptr : &pack->second;
			std::error_code sourceError;
			const bool levelSourceReady = std::filesystem::exists(level.sourcePath, sourceError) && !sourceError;
			const bool packSourceReady = assetPack == nullptr || m_catalog.IsAssetPackSourceReady(assetPack->id);
			const bool runtimeSupported = assetPack == nullptr || assetPack->runtimeSupported;
			const bool acquisitionSupported = assetPack != nullptr && assetPack->downloadSupported;
			const bool sourceReady = levelSourceReady && packSourceReady;
			const bool canSelect =
			    levelSourceReady && runtimeSupported && (assetPack == nullptr || packSourceReady || acquisitionSupported);
			LauncherLevelUiEntry entry{
			    .Id = QString::fromStdString(level.id),
			    .DisplayName = DisplayNameOrId(level.displayName, level.id),
			    .Description = QString::fromStdString(level.description),
			    .ThumbnailPath = QString::fromStdString(level.thumbnailPath.string()),
			    .Detail = BuildLevelDetail(level, assetPack),
			    .SourcePageUrl = QString::fromStdString(
			        level.sourcePageUrl.empty() && assetPack != nullptr ? assetPack->sourcePageUrl : level.sourcePageUrl),
			    .UnsupportedReason = ResolveUnavailableReason(level, assetPack, levelSourceReady, canSelect),
			    .Selected = level.selected,
			    .SourceReady = sourceReady,
			    .RuntimeSupported = runtimeSupported,
			    .Ready = m_catalog.IsLevelReady(level),
			    .CanSelect = canSelect};

			entry.Status = ResolveLevelStatus(entry);
			entry.State = ResolveLevelState(entry);
			return entry;
		}

		QString ResolveUnavailableReason(
		    const ProjectLevelCatalogEntry& level,
		    const ProjectAssetPack* pack,
		    bool levelSourceReady,
		    bool canSelect) const
		{
			if (canSelect)
			{
				return {};
			}
			if (!levelSourceReady)
			{
				return QStringLiteral("Level definition is missing: %1").arg(QString::fromStdString(level.sourcePath.string()));
			}
			if (pack == nullptr)
			{
				return QStringLiteral("This repository map has no available source content.");
			}
			if (!pack->runtimeSupported)
			{
				return QString::fromStdString(pack->runtimeBlocker);
			}
			return QString::fromStdString(pack->downloadBlocker);
		}

		LauncherStartupLevelUiEntry BuildStartupLevel(const ProjectLevelCatalogEntry& level) const
		{
			LauncherStartupLevelUiEntry entry{
			    .Id = QString::fromStdString(level.id),
			    .DisplayName = DisplayNameOrId(level.displayName, level.id),
			    .Ready = m_catalog.IsLevelReady(level)};

			entry.Status = entry.Ready ? "ready" : "missing source content";
			return entry;
		}

		QString BuildLevelDetail(const ProjectLevelCatalogEntry& level, const ProjectAssetPack* pack) const
		{
			QStringList traits;
			if (level.variantKind == "SceneVariant")
			{
				traits.push_back("variant");
			}
			else if (level.variantKind == "AddOn")
			{
				traits.push_back("add-on");
			}
			else if (level.variantKind == "Base")
			{
				traits.push_back("base map");
			}

			if (pack != nullptr)
			{
				const std::uintmax_t archiveBytes = ArchiveChainBytes(pack);
				if (archiveBytes > 0)
				{
					traits.push_back(FormatArchiveSize(archiveBytes) + " download");
				}
			}

			return traits.empty() ? QStringLiteral("Repository map") : traits.join("  ·  ");
		}

		std::uintmax_t ArchiveChainBytes(const ProjectAssetPack* pack) const
		{
			std::uintmax_t total = 0;
			while (pack != nullptr)
			{
				if (pack->downloadSupported)
				{
					if (pack->archiveBytes > std::numeric_limits<std::uintmax_t>::max() - total)
					{
						return std::numeric_limits<std::uintmax_t>::max();
					}
					total += pack->archiveBytes;
				}
				if (pack->parentPackId.empty())
				{
					break;
				}

				const auto parent = m_catalog.assetPacks.find(pack->parentPackId);
				pack = parent == m_catalog.assetPacks.end() ? nullptr : &parent->second;
			}
			return total;
		}

		QString ResolveLevelStatus(const LauncherLevelUiEntry& level) const
		{
			if (!level.RuntimeSupported)
			{
				return "Future";
			}
			if (!level.CanSelect)
			{
				return "Unavailable";
			}
			if (level.Selected && level.Ready)
			{
				return "Ready";
			}
			if (level.Selected)
			{
				return "Download required";
			}
			return level.SourceReady ? QStringLiteral("Source ready") : QStringLiteral("Available to sync");
		}

		QString ResolveLevelState(const LauncherLevelUiEntry& level) const
		{
			if (!level.RuntimeSupported || !level.CanSelect)
			{
				return "warning";
			}
			if (level.Selected && level.Ready)
			{
				return "ok";
			}
			if (level.Selected)
			{
				return "warning";
			}
			return level.SourceReady ? QStringLiteral("ok") : QStringLiteral("neutral");
		}

		QString DisplayNameOrId(std::string_view displayName, std::string_view id) const
		{
			return QString::fromStdString(std::string(displayName.empty() ? id : displayName));
		}

		QString FormatArchiveSize(std::uintmax_t byteCount) const
		{
			constexpr double bytesPerGibibyte = 1024.0 * 1024.0 * 1024.0;
			constexpr double bytesPerMebibyte = 1024.0 * 1024.0;
			std::ostringstream stream;
			stream << std::fixed << std::setprecision(2);
			if (byteCount >= static_cast<std::uintmax_t>(bytesPerGibibyte))
			{
				stream << static_cast<double>(byteCount) / bytesPerGibibyte << " GiB";
			}
			else
			{
				stream << static_cast<double>(byteCount) / bytesPerMebibyte << " MiB";
			}
			return QString::fromStdString(stream.str());
		}

		const LauncherProjectSummary& m_project;
		ProjectLevelCatalog m_catalog;
		LauncherLevelUiModel m_model;
	};

	LauncherLevelUiModel LauncherLevelUiModel::Build(const LauncherProjectSummary& project)
	{
		return LauncherLevelUiModelBuilder(project).Build();
	}
}
