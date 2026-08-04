#include "LauncherDependencyUiModel.h"

#include "SparkleLauncher/SourceDependencyState.h"

#include <string_view>

namespace SparkleLauncher
{
	static QString ToQString(std::string_view text)
	{
		return QString::fromStdString(std::string(text));
	}

	static ThirdPartyDependencyUiEntry ToUiDependency(const SourceDependencyEntry& dependency)
	{
		return {
		    ToQString(dependency.Id),
		    ToQString(dependency.Label),
		    ToQString(dependency.Version),
		    ToQString(dependency.Purpose),
		    ToQString(dependency.CacheDirectoryName),
		    dependency.Required,
		    dependency.Enabled,
		};
	}

	static SourceDependencyValidation ValidateDependency(
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

	const std::vector<ThirdPartyDependencyUiEntry>& GetTrackedThirdPartyDependencies()
	{
		static const std::vector<ThirdPartyDependencyUiEntry> dependencies = []
		{
			std::vector<ThirdPartyDependencyUiEntry> entries;
			for (const SourceDependencyEntry& dependency : GetSourceDependencies())
			{
				entries.push_back(ToUiDependency(dependency));
			}
			return entries;
		}();
		return dependencies;
	}

	ThirdPartyDependencyUiStatus BuildThirdPartyDependencyStatus(
	    const ThirdPartyDependencyUiEntry& dependency,
	    const std::filesystem::path& dependencyCachePath)
	{
		const SourceDependencyValidation validation = ValidateDependency(dependency, dependencyCachePath);
		const QString detail =
		    dependency.Purpose.isEmpty() ? dependency.Version : QStringLiteral("%1 · %2").arg(dependency.Version, dependency.Purpose);
		return validation.Ready ? ThirdPartyDependencyUiStatus{"Synced", detail, "ok", true}
		                        : ThirdPartyDependencyUiStatus{"Missing", detail, "warning", false};
	}
}
