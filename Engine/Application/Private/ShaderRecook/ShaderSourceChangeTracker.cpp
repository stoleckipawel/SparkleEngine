#include "PCH.h"

#include "ShaderRecook/ShaderSourceChangeTracker.h"

#include "Core/Public/Paths/DirectoryPaths.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <system_error>
#include <utility>

bool ShaderSourceChangeTracker::HasChanged() noexcept
{
	using namespace std::chrono_literals;
	const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	if (now < m_nextScanTime)
	{
		return false;
	}
	m_nextScanTime = now + 500ms;

	if (!m_hasPrimedSnapshot)
	{
		PrimeSnapshot();
		return false;
	}

	return RefreshSnapshot(true);
}

void ShaderSourceChangeTracker::PrimeSnapshot() noexcept
{
	RefreshSnapshot(false);
	m_hasPrimedSnapshot = true;
}

bool ShaderSourceChangeTracker::RefreshSnapshot(bool detectChanges) noexcept
{
	bool changed = false;
	std::unordered_map<std::string, std::filesystem::file_time_type> currentWriteTimes;

	const std::array<std::filesystem::path, 2> shaderRoots{
	    Paths::ShaderSourceRoot(PathRoot::Project),
	    Paths::ShaderSourceRoot(PathRoot::Engine)};

	for (const std::filesystem::path& shaderRoot : shaderRoots)
	{
		if (shaderRoot.empty())
		{
			continue;
		}

		std::error_code errorCode;
		if (!std::filesystem::exists(shaderRoot, errorCode) || errorCode)
		{
			continue;
		}

		for (std::filesystem::recursive_directory_iterator it(shaderRoot, errorCode), end; it != end && !errorCode; it.increment(errorCode))
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

			const std::string pathKey = it->path().lexically_normal().generic_string();
			currentWriteTimes.emplace(pathKey, writeTime);
			if (detectChanges)
			{
				const auto previous = m_writeTimes.find(pathKey);
				if (previous == m_writeTimes.end() || previous->second != writeTime)
				{
					changed = true;
				}
			}
		}
	}

	if (detectChanges && currentWriteTimes.size() != m_writeTimes.size())
	{
		changed = true;
	}
	m_writeTimes = std::move(currentWriteTimes);
	return changed;
}

bool ShaderSourceChangeTracker::IsWatchedShaderSource(const std::filesystem::path& path) noexcept
{
	std::string extension = path.extension().generic_string();
	std::ranges::transform(
	    extension,
	    extension.begin(),
	    [](unsigned char value)
	    {
		    return static_cast<char>(std::tolower(value));
	    });

	return extension == ".hlsl" || extension == ".hlsli" || extension == ".slang";
}
