#include "LauncherPageUtilities.h"

#include <system_error>

namespace SparkleLauncher
{
	QString ToDisplayPath(
	    const std::filesystem::path& repositoryRoot,
	    const std::filesystem::path& path)
	{
		std::error_code errorCode;
		const std::filesystem::path relative = std::filesystem::relative(path, repositoryRoot, errorCode);
		return QString::fromStdString(
		    (!errorCode && !relative.empty()) ? relative.generic_string() : path.generic_string());
	}

	QString FormatDirectoryInventory(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::exists(path, errorCode) || errorCode)
		{
			return "not present";
		}
		if (std::filesystem::is_regular_file(path, errorCode))
		{
			return "1 file";
		}

		std::uintmax_t fileCount = 0;
		std::uintmax_t directoryCount = 0;
		if (std::filesystem::is_directory(path, errorCode))
		{
			std::filesystem::recursive_directory_iterator iterator(
			    path,
			    std::filesystem::directory_options::skip_permission_denied,
			    errorCode);
			const std::filesystem::recursive_directory_iterator end;
			while (iterator != end)
			{
				const std::filesystem::directory_entry entry = *iterator;
				if (entry.is_directory(errorCode))
				{
					++directoryCount;
				}
				else if (entry.is_regular_file(errorCode))
				{
					++fileCount;
				}
				errorCode.clear();
				iterator.increment(errorCode);
				errorCode.clear();
			}
		}

		return QStringLiteral("%1 files, %2 folders").arg(fileCount).arg(directoryCount);
	}

	QString FormatStatusPath(const std::filesystem::path& path)
	{
		return path.empty() ? QString() : QString::fromStdString(path.string());
	}

	bool DirectoryHasEntries(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		if (!std::filesystem::is_directory(path, errorCode))
		{
			return false;
		}
		return std::filesystem::directory_iterator(path, errorCode) !=
		       std::filesystem::directory_iterator();
	}

	bool PathExists(const std::filesystem::path& path)
	{
		std::error_code errorCode;
		return std::filesystem::exists(path, errorCode) && !errorCode;
	}

	bool ReadinessContains(const std::vector<std::string>& messages, const QString& needle)
	{
		for (const std::string& message : messages)
		{
			if (QString::fromStdString(message).contains(needle, Qt::CaseInsensitive))
			{
				return true;
			}
		}
		return false;
	}

	QString CombineStatusDetail(const QString& first, const QString& second)
	{
		if (first.isEmpty())
		{
			return second;
		}
		if (second.isEmpty())
		{
			return first;
		}
		return first + " | " + second;
	}

	QString BuildFilesRecoveryHint(const BuildFilesFreshnessStatus& freshness)
	{
		switch (freshness.State)
		{
			case BuildFilesFreshnessState::GeneratorMismatch:
			case BuildFilesFreshnessState::FeatureSetMismatch:
				return "Recovery: clean Build Outputs or choose a different build directory before running Generate Build Files again.";
			case BuildFilesFreshnessState::BuildDirectoryMissing:
			case BuildFilesFreshnessState::CMakeCacheMissing:
			case BuildFilesFreshnessState::SolutionMissing:
			case BuildFilesFreshnessState::FreshnessStampMissing:
			case BuildFilesFreshnessState::FreshnessStampMismatch:
			case BuildFilesFreshnessState::SourceListChanged:
			case BuildFilesFreshnessState::BuildInputChanged:
				return "Recovery: run Generate Build Files to refresh generated CMake and IDE state.";
			case BuildFilesFreshnessState::Current:
			case BuildFilesFreshnessState::Unsupported:
				return {};
		}
		return {};
	}
}  // namespace SparkleLauncher
