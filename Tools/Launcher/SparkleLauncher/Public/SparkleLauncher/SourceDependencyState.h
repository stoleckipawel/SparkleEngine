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
		bool Required = false;
		bool Enabled = false;
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

	const std::vector<SourceDependencyEntry>& GetSourceDependencies();
	const SourceDependencyEntry* FindSourceDependency(std::string_view id);
	SourceDependencyValidation ValidateSourceDependency(
	    const SourceDependencyEntry& dependency,
	    const std::filesystem::path& dependencyCacheRoot);
	std::vector<std::filesystem::path> GetSourceDependencyCachePaths(
	    const SourceDependencyEntry& dependency,
	    const std::filesystem::path& dependencyCacheRoot);
	SourceDependencyInventoryStatus InspectSourceDependencyCache(const std::filesystem::path& dependencyCacheRoot);
}
