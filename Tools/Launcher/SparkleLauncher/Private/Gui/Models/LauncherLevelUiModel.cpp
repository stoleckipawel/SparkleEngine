#include "LauncherLevelUiModel.h"

#include "LauncherProjectModel.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Projects/ProjectLevelCatalog.h"

#include <QtCore/QStringList>

#include <filesystem>
#include <iomanip>
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
		explicit LauncherLevelUiModelBuilder(const LauncherProjectSummary& project) noexcept;

		LauncherLevelUiModel Build();

	private:
		void AppendLevels();
		void AppendContentPacks();
		void AppendStartupLevels(bool startupDefault);
		LauncherLevelUiEntry BuildLevel(const ProjectLevelCatalogEntry& level) const;
		LauncherContentPackUiEntry BuildContentPack(const ProjectOptionalContentPack& pack) const;
		LauncherStartupLevelUiEntry BuildStartupLevel(const ProjectLevelCatalogEntry& level) const;
		QString BuildLevelDetail(const ProjectLevelCatalogEntry& level) const;
		QString BuildContentPackDetail(const ProjectOptionalContentPack& pack) const;
		QString ResolveLevelStatus(const LauncherLevelUiEntry& level) const;
		QString ResolveLevelState(const LauncherLevelUiEntry& level) const;
		QString ResolveContentPackStatus(const LauncherContentPackUiEntry& pack) const;
		QString ResolveContentPackState(const LauncherContentPackUiEntry& pack) const;
		QString DisplayNameOrId(std::string_view displayName, std::string_view id) const;
		QString RelativeProjectPath(const std::filesystem::path& path) const;
		QString FormatArchiveSize(std::uintmax_t byteCount) const;

		const LauncherProjectSummary& m_project;
		ProjectLevelCatalog m_catalog;
		LauncherLevelUiModel m_model;
	};

	LauncherLevelUiModel LauncherLevelUiModel::Build(const LauncherProjectSummary& project)
	{
		return LauncherLevelUiModelBuilder(project).Build();
	}

	LauncherLevelUiModelBuilder::LauncherLevelUiModelBuilder(const LauncherProjectSummary& project) noexcept :
	    m_project(project)
	{
	}

	LauncherLevelUiModel LauncherLevelUiModelBuilder::Build()
	{
		try
		{
			m_catalog = ProjectLevelCatalogFile::Load(m_project.RootPath);
			m_model.Loaded = true;
		}
		catch (const Diagnostics::Error&)
		{
			return std::move(m_model);
		}

		AppendLevels();
		AppendContentPacks();
		AppendStartupLevels(true);
		AppendStartupLevels(false);
		return std::move(m_model);
	}

	void LauncherLevelUiModelBuilder::AppendLevels()
	{
		m_model.Levels.reserve(static_cast<qsizetype>(m_catalog.levels.size()));
		for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
		{
			m_model.Levels.push_back(BuildLevel(level));
		}
	}

	void LauncherLevelUiModelBuilder::AppendContentPacks()
	{
		m_model.ContentPacks.reserve(static_cast<qsizetype>(m_catalog.optionalContentPacks.size()));
		for (const auto& contentPack : m_catalog.optionalContentPacks)
		{
			m_model.ContentPacks.push_back(BuildContentPack(contentPack.second));
		}
	}

	void LauncherLevelUiModelBuilder::AppendStartupLevels(bool startupDefault)
	{
		for (const ProjectLevelCatalogEntry& level : m_catalog.levels)
		{
			if (level.startupDefault == startupDefault)
			{
				m_model.StartupLevels.push_back(BuildStartupLevel(level));
			}
		}
	}

	LauncherLevelUiEntry LauncherLevelUiModelBuilder::BuildLevel(const ProjectLevelCatalogEntry& level) const
	{
		const auto contentPack = level.optionalContentPackId.empty() ? m_catalog.optionalContentPacks.end()
		                                                             : m_catalog.optionalContentPacks.find(level.optionalContentPackId);
		const bool selectable = contentPack == m_catalog.optionalContentPacks.end() || contentPack->second.runtimeSupported;
		LauncherLevelUiEntry entry{
		    .Id = QString::fromStdString(level.id),
		    .DisplayName = DisplayNameOrId(level.displayName, level.id),
		    .Detail = BuildLevelDetail(level),
		    .Family = QString::fromStdString(level.family),
		    .UnsupportedReason = selectable ? QString() : QString::fromStdString(contentPack->second.runtimeBlocker),
		    .Synced = level.defaultIncluded,
		    .Ready = m_catalog.IsLevelReady(m_project.RootPath, level),
		    .Selectable = selectable,
		    .StartupDefault = level.startupDefault};

		entry.Status = ResolveLevelStatus(entry);
		entry.State = ResolveLevelState(entry);
		return entry;
	}

	LauncherContentPackUiEntry LauncherLevelUiModelBuilder::BuildContentPack(const ProjectOptionalContentPack& pack) const
	{
		LauncherContentPackUiEntry entry{
		    .Id = QString::fromStdString(pack.id),
		    .DisplayName = DisplayNameOrId(pack.displayName, pack.id),
		    .Detail = BuildContentPackDetail(pack),
		    .ParentPackId = QString::fromStdString(pack.parentPackId),
		    .SourcePageUrl = QString::fromStdString(pack.sourcePageUrl),
		    .DownloadBlocker = QString::fromStdString(pack.downloadBlocker),
		    .Selected = pack.available,
		    .Acquired = m_catalog.IsOptionalContentPackAcquired(pack),
		    .DownloadSupported = pack.downloadSupported,
		    .RuntimeSupported = pack.runtimeSupported};

		entry.Status = ResolveContentPackStatus(entry);
		entry.State = ResolveContentPackState(entry);
		return entry;
	}

	LauncherStartupLevelUiEntry LauncherLevelUiModelBuilder::BuildStartupLevel(const ProjectLevelCatalogEntry& level) const
	{
		LauncherStartupLevelUiEntry entry{
		    .Id = QString::fromStdString(level.id),
		    .DisplayName = DisplayNameOrId(level.displayName, level.id),
		    .Synced = level.defaultIncluded,
		    .Ready = m_catalog.IsLevelReady(m_project.RootPath, level),
		    .StartupDefault = level.startupDefault};

		entry.Status = !entry.Synced ? "not synced" : entry.Ready ? "synced" : "missing";
		return entry;
	}

	QString LauncherLevelUiModelBuilder::BuildLevelDetail(const ProjectLevelCatalogEntry& level) const
	{
		QStringList traits;
		if (level.defaultIncluded)
		{
			traits.push_back("synced");
		}
		if (level.startupDefault)
		{
			traits.push_back("startup default");
		}
		if (!level.optionalContentPackId.empty())
		{
			traits.push_back(QStringLiteral("pack %1").arg(QString::fromStdString(level.optionalContentPackId)));
		}
		if (!level.variantKind.empty())
		{
			traits.push_back(QString::fromStdString(level.variantKind == "AddOn" ? "Modern Sponza add-on variant" : level.variantKind));
		}

		return QStringLiteral("%1%2")
		    .arg(RelativeProjectPath(level.sourcePath))
		    .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", ")));
	}

	QString LauncherLevelUiModelBuilder::BuildContentPackDetail(const ProjectOptionalContentPack& pack) const
	{
		QStringList traits;
		if (pack.external)
		{
			traits.push_back("external");
		}
		if (!pack.parentPackId.empty())
		{
			traits.push_back(QStringLiteral("add-on for %1").arg(QString::fromStdString(pack.parentPackId)));
		}
		if (pack.archiveBytes > 0)
		{
			traits.push_back(FormatArchiveSize(pack.archiveBytes));
		}
		if (!pack.version.empty())
		{
			traits.push_back(QString::fromStdString(pack.version));
		}
		if (!pack.license.empty())
		{
			traits.push_back(QString::fromStdString(pack.license));
		}

		QString detail = QStringLiteral("%1%2")
		                     .arg(RelativeProjectPath(pack.rootPath))
		                     .arg(traits.empty() ? QString() : QStringLiteral(" | %1").arg(traits.join(", ")));
		if (!pack.downloadSupported && !pack.downloadBlocker.empty())
		{
			detail += QStringLiteral(" Future support: %1").arg(QString::fromStdString(pack.downloadBlocker));
		}
		else if (!pack.runtimeSupported && !pack.runtimeBlocker.empty())
		{
			detail +=
			    QStringLiteral(" Source sync is available; runtime use is blocked: %1").arg(QString::fromStdString(pack.runtimeBlocker));
		}

		return detail;
	}

	QString LauncherLevelUiModelBuilder::ResolveLevelStatus(const LauncherLevelUiEntry& level) const
	{
		if (!level.Selectable)
		{
			return "Conversion pending";
		}
		if (!level.Synced)
		{
			return "Off";
		}
		if (!level.Ready)
		{
			return "Missing";
		}

		return "Synced";
	}

	QString LauncherLevelUiModelBuilder::ResolveLevelState(const LauncherLevelUiEntry& level) const
	{
		if (!level.Selectable || !level.Synced)
		{
			return "neutral";
		}
		if (level.Ready)
		{
			return "ok";
		}

		return "bad";
	}

	QString LauncherLevelUiModelBuilder::ResolveContentPackStatus(const LauncherContentPackUiEntry& pack) const
	{
		if (!pack.DownloadSupported)
		{
			return pack.DownloadBlocker.isEmpty() ? (pack.Acquired ? "Ready" : "Manual") : "Future";
		}
		if (pack.Acquired && !pack.RuntimeSupported)
		{
			return "Source ready";
		}
		if (pack.Acquired)
		{
			return "Ready";
		}
		return pack.Selected ? "Selected" : "Optional";
	}

	QString LauncherLevelUiModelBuilder::ResolveContentPackState(const LauncherContentPackUiEntry& pack) const
	{
		if (!pack.DownloadSupported)
		{
			return pack.DownloadBlocker.isEmpty() && pack.Acquired ? "ok" : "neutral";
		}
		if (pack.Acquired)
		{
			return pack.RuntimeSupported ? "ok" : "warning";
		}
		return pack.Selected ? "warning" : "neutral";
	}

	QString LauncherLevelUiModelBuilder::DisplayNameOrId(std::string_view displayName, std::string_view id) const
	{
		return QString::fromStdString(std::string(displayName.empty() ? id : displayName));
	}

	QString LauncherLevelUiModelBuilder::RelativeProjectPath(const std::filesystem::path& path) const
	{
		if (path.empty())
		{
			return {};
		}

		std::error_code error;
		const std::filesystem::path relative = std::filesystem::relative(path, m_project.RootPath, error);
		return QString::fromStdString((error ? path : relative).generic_string());
	}

	QString LauncherLevelUiModelBuilder::FormatArchiveSize(std::uintmax_t byteCount) const
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
}
