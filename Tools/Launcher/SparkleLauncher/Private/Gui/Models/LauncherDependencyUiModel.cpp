#include "LauncherDependencyUiModel.h"

#include "SparkleLauncher/SourceDependencyState.h"

#include <QtCore/QStringList>

#include <string_view>
#include <utility>

namespace SparkleLauncher
{

	QString ToQString(std::string_view text)
	{
		return QString::fromStdString(std::string(text));
	}

	ThirdPartyDependencyUiEntry ToUiDependency(const SourceDependencyEntry& dependency)
	{
		return {
		    ToQString(dependency.Id),
		    ToQString(dependency.Label),
		    ToQString(dependency.Version),
		    ToQString(dependency.Purpose),
		    ToQString(dependency.CacheDirectoryName),
		};
	}

	SourceDependencyValidation ValidateDependency(
	    const ThirdPartyDependencyUiEntry& dependency,
	    const std::filesystem::path& dependencyCachePath)
	{
		if (const SourceDependencyEntry* sourceDependency = FindSourceDependency(dependency.Id.toStdString()))
		{
			return ValidateSourceDependency(*sourceDependency, dependencyCachePath);
		}

		SourceDependencyValidation validation;
		validation.CachePath = dependencyCachePath / dependency.CacheDirectoryName.toStdString();
		validation.MissingRelativePaths.push_back(dependency.CacheDirectoryName.toStdString());
		return validation;
	}

	QString FormatMissingRelativePaths(const std::vector<std::string>& missingRelativePaths)
	{
		QStringList parts;
		for (const std::string& path : missingRelativePaths)
		{
			parts.push_back(QString::fromStdString(path));
		}
		return parts.join(", ");
	}

	QString FormatIncompleteDependencyLabels(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		QStringList labels;
		for (const ThirdPartyDependencyUiEntry& dependency : group.Dependencies)
		{
			if (!ValidateDependency(dependency, dependencyCachePath).Ready)
			{
				labels.push_back(dependency.Label);
			}
		}
		return labels.join(", ");
	}

	const std::vector<DependencyGroupUiEntry>& GetDependencyGroups()
	{
		static const std::vector<DependencyGroupUiEntry> groups = []
		{
			std::vector<DependencyGroupUiEntry> entries;
			for (const SourceDependencyGroup& group : GetSourceDependencyGroups())
			{
				DependencyGroupUiEntry entry;
				entry.Id = ToQString(group.Id);
				entry.Label = ToQString(group.Label);
				entry.Summary = ToQString(group.Summary);
				entry.UnlockSummary = ToQString(group.UnlockSummary);
				entry.ConfigureOption = ToQString(group.ConfigureOption);
				entry.EnablementDetail = ToQString(group.EnablementDetail);
				entry.Required = group.Required;
				entry.Enabled = group.Enabled;
				for (const SourceDependencyEntry& dependency : group.Dependencies)
				{
					entry.Dependencies.push_back(ToUiDependency(dependency));
				}
				entries.push_back(std::move(entry));
			}
			return entries;
		}();
		return groups;
	}

	const std::vector<ThirdPartyDependencyUiEntry>& GetTrackedThirdPartyDependencies()
	{
		static const std::vector<ThirdPartyDependencyUiEntry> dependencies = []
		{
			std::vector<ThirdPartyDependencyUiEntry> entries;
			for (const DependencyGroupUiEntry& group : GetDependencyGroups())
			{
				entries.insert(entries.end(), group.Dependencies.begin(), group.Dependencies.end());
			}
			return entries;
		}();
		return dependencies;
	}

	QString FormatTrackedDependencySummary(const std::filesystem::path& dependencyCachePath)
	{
		const SourceDependencyInventoryStatus status = InspectSourceDependencyCache(dependencyCachePath);
		const int readyCount = status.ReadyDependencyCount;
		const int trackedCount = status.EnabledDependencyCount;
		return QStringLiteral("%1 of %2 enabled tracked dependencies are cached.").arg(readyCount).arg(trackedCount);
	}

	int CountReadyDependencies(const DependencyGroupUiEntry& group, const std::filesystem::path& dependencyCachePath)
	{
		if (const SourceDependencyGroup* sourceGroup = FindSourceDependencyGroup(group.Id.toStdString()))
		{
			return CountReadySourceDependencies(*sourceGroup, dependencyCachePath);
		}
		return 0;
	}

	QString DependencyGroupStatusText(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "Disabled";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return group.Required ? "Ready" : "Cached";
		}
		if (readyCount > 0)
		{
			return "Partial";
		}
		return group.Required ? "Pending sync" : "Available";
	}

	QString DependencyGroupStatusState(const DependencyGroupUiEntry& group, int readyCount)
	{
		if (!group.Enabled)
		{
			return "neutral";
		}
		if (readyCount == static_cast<int>(group.Dependencies.size()))
		{
			return "ok";
		}
		return "warning";
	}

	QString FormatDependencyGroupDetail(
	    const DependencyGroupUiEntry& group,
	    const std::filesystem::path& dependencyCachePath,
	    int readyCount)
	{
		QString detail = group.Summary + " " + group.UnlockSummary;
		if (!group.Enabled)
		{
			return detail + (group.EnablementDetail.isEmpty() ? QString() : QStringLiteral(" %1").arg(group.EnablementDetail));
		}
		detail += QStringLiteral(" %1 of %2 tracked dependencies are cached.").arg(readyCount).arg(group.Dependencies.size());
		if (readyCount < static_cast<int>(group.Dependencies.size()))
		{
			const QString incompleteLabels = FormatIncompleteDependencyLabels(group, dependencyCachePath);
			if (!incompleteLabels.isEmpty())
			{
				detail += QStringLiteral(" Incomplete dependencies: %1.").arg(incompleteLabels);
			}
		}
		return detail;
	}

	bool OperationUsesDependencyGroup(const QString& operationId, const DependencyGroupUiEntry& group)
	{
		if (operationId == "workspace.sync-source-tiers" || operationId == "workspace.sync-all")
		{
			return true;
		}
		if (group.Id == "core-workspace")
		{
			return operationId == "workspace.generate-build-files" || operationId == "workspace.open-ide"
			    || operationId == "workspace.build-all" || operationId == "launcher.build.self" || operationId.startsWith("project.build")
			    || operationId.startsWith("cook.");
		}
		if (group.Id == "content-pipeline")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.textures"
			    || operationId == "cook.assets" || operationId == "cook.project";
		}
		if (group.Id == "shader-compiler")
		{
			return operationId == "workspace.build-all" || operationId == "cook.tools.prepare" || operationId == "cook.shaders"
			    || operationId == "cook.project";
		}
		if (group.Id == "ktx-support")
		{
			return operationId == "workspace.sync-source-tiers" || operationId == "cook.textures" || operationId == "cook.project";
		}
		if (group.Id == "nvidia-streamline")
		{
			return operationId == "workspace.sync-source-tiers" || operationId == "workspace.generate-build-files"
			    || operationId == "workspace.build-all" || operationId.startsWith("project.build");
		}
		return false;
	}
}
