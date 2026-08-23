#include "PCH.h"
#include "Core/Public/FileSystemUtils.h"

#include "ShaderRecook/ShaderSourceChangeTracker.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>
#include <utility>

std::vector<std::string> ShaderSourceChangeTracker::CollectChangedVirtualPaths() noexcept
{
	using namespace std::chrono_literals;
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (now < m_nextScanTime)
	{
		return {};
	}
	m_nextScanTime = now + 500ms;

	if (!m_hasPrimedSnapshot)
	{
		PrimeSnapshot();
		return {};
	}

	return RefreshSnapshot(true);
}

void ShaderSourceChangeTracker::PrimeSnapshot() noexcept
{
	(void) RefreshSnapshot(false);
	m_hasPrimedSnapshot = true;
}

std::vector<std::string> ShaderSourceChangeTracker::RefreshSnapshot(bool detectChanges) noexcept
{
	std::vector<std::string> changedVirtualPaths;
	std::unordered_map<std::string, std::filesystem::file_time_type> currentWriteTimes;

	struct WatchedRoot final
	{
		std::filesystem::path PhysicalPath;
		std::string_view VirtualPath;
	};
	const std::array<WatchedRoot, 2> shaderRoots{
	    WatchedRoot{Filesystem::GetShaderPath(PathRoot::Project), "/Project"},
	    WatchedRoot{Filesystem::GetShaderPath(PathRoot::Engine), "/Engine"}};

	for (const WatchedRoot& shaderRoot : shaderRoots)
	{
		if (shaderRoot.PhysicalPath.empty())
		{
			continue;
		}

		std::error_code errorCode;
		if (!std::filesystem::exists(shaderRoot.PhysicalPath, errorCode) || errorCode)
		{
			continue;
		}

		for (std::filesystem::recursive_directory_iterator it(shaderRoot.PhysicalPath, errorCode), end; it != end && !errorCode;
		    it.increment(errorCode))
		{
			if (!it->is_regular_file(errorCode) || errorCode || !IsWatchedShaderSource(it->path()))
			{
				errorCode.clear();
				continue;
			}

			const std::filesystem::file_time_type writeTime = it->last_write_time(errorCode);
			if (errorCode)
			{
				errorCode.clear();
				continue;
			}

			const std::string pathKey = BuildVirtualPath(shaderRoot.PhysicalPath, it->path(), shaderRoot.VirtualPath);
			if (pathKey.empty())
			{
				continue;
			}
			currentWriteTimes.emplace(pathKey, writeTime);
			if (detectChanges)
			{
				const auto previous = m_writeTimes.find(pathKey);
				if (previous == m_writeTimes.end() || previous->second != writeTime)
				{
					changedVirtualPaths.push_back(pathKey);
				}
			}
		}
	}

	if (detectChanges)
	{
		for (const auto& [pathKey, writeTime] : m_writeTimes)
		{
			(void) writeTime;
			if (!currentWriteTimes.contains(pathKey))
			{
				changedVirtualPaths.push_back(pathKey);
			}
		}
	}
	m_writeTimes = std::move(currentWriteTimes);
	std::ranges::sort(changedVirtualPaths);
	changedVirtualPaths.erase(std::unique(changedVirtualPaths.begin(), changedVirtualPaths.end()), changedVirtualPaths.end());
	return changedVirtualPaths;
}

std::string ShaderSourceChangeTracker::BuildVirtualPath(
    const std::filesystem::path& shaderRoot,
    const std::filesystem::path& sourcePath,
    std::string_view virtualRoot) noexcept
{
	std::error_code errorCode;
	const std::filesystem::path relativePath = std::filesystem::relative(sourcePath, shaderRoot, errorCode);
	if (errorCode || relativePath.empty())
	{
		return {};
	}
	return std::string(virtualRoot) + '/' + relativePath.generic_string();
}

bool ShaderSourceChangeTracker::IsWatchedShaderSource(const std::filesystem::path& path) noexcept
{
	std::string extension = path.extension().generic_string();
	std::ranges::transform(extension, extension.begin(), [](unsigned char value) { return static_cast<char>(std::tolower(value)); });

	return extension == ".hlsl" || extension == ".hlsli" || extension == ".slang";
}
