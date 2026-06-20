#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace SparkleLauncher
{
	struct SourceDependencyEntry
	{
		std::string Id;
		std::string Label;
		std::string Version;
		std::string Purpose;
		std::string CacheDirectoryName;
		std::vector<std::string> RequiredRelativePaths;
	};

	struct SourceDependencyGroup
	{
		std::string Id;
		std::string Label;
		std::string Summary;
		std::string UnlockSummary;
		std::string ConfigureOption;
		std::string EnablementDetail;
		bool Required = false;
		bool Enabled = false;
		std::vector<SourceDependencyEntry> Dependencies;
	};

	struct SourceDependencyValidation
	{
		std::filesystem::path CachePath;
		bool Ready = false;
		std::vector<std::string> MissingRelativePaths;
	};

	struct SourceDependencyInventoryStatus
	{
		std::filesystem::path CacheRoot;
		int EnabledDependencyCount = 0;
		int ReadyDependencyCount = 0;
		bool AllEnabledDependenciesReady = true;
		std::vector<std::string> ReadinessMessages;
	};

	const std::vector<SourceDependencyGroup>& GetSourceDependencyGroups();
	const SourceDependencyGroup* FindSourceDependencyGroup(std::string_view id);
	const SourceDependencyEntry* FindSourceDependency(std::string_view id);
	SourceDependencyValidation ValidateSourceDependency(
	    const SourceDependencyEntry& dependency,
	    const std::filesystem::path& dependencyCacheRoot);
	int CountReadySourceDependencies(const SourceDependencyGroup& group, const std::filesystem::path& dependencyCacheRoot);
	SourceDependencyInventoryStatus InspectSourceDependencyCache(const std::filesystem::path& dependencyCacheRoot);
}
