#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

class ShaderSourceChangeTracker final
{
public:
	std::vector<std::string> CollectChangedVirtualPaths() noexcept;

private:
	void PrimeSnapshot() noexcept;
	std::vector<std::string> RefreshSnapshot(bool detectChanges) noexcept;
	static bool IsWatchedShaderSource(const std::filesystem::path& path) noexcept;
	static std::string BuildVirtualPath(
	    const std::filesystem::path& shaderRoot,
	    const std::filesystem::path& sourcePath,
	    std::string_view virtualRoot) noexcept;

	std::chrono::steady_clock::time_point m_nextScanTime{};
	std::unordered_map<std::string, std::filesystem::file_time_type> m_writeTimes;
	bool m_hasPrimedSnapshot = false;
};
