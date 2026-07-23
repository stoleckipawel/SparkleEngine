#pragma once

#include "SparkleLauncher/BuildWorkspaceOperations.h"

#include <QtCore/QString>

#include <filesystem>
#include <string>
#include <vector>

namespace SparkleLauncher
{
	QString ToDisplayPath(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& path);
	QString FormatDirectoryInventory(const std::filesystem::path& path);
	QString FormatStatusPath(const std::filesystem::path& path);
	bool DirectoryHasEntries(const std::filesystem::path& path);
	bool PathExists(const std::filesystem::path& path);
	bool ReadinessContains(const std::vector<std::string>& messages, const QString& needle);
	QString CombineStatusDetail(const QString& first, const QString& second);
	QString BuildFilesRecoveryHint(const BuildFilesFreshnessStatus& freshness);
}  // namespace SparkleLauncher
