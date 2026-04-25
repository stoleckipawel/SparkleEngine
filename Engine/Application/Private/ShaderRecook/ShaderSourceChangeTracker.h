#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>

class ShaderSourceChangeTracker final
{
  public:
	bool HasChanged() noexcept;

  private:
	void PrimeSnapshot() noexcept;
	bool RefreshSnapshot(bool detectChanges) noexcept;
	static bool IsWatchedShaderSource(const std::filesystem::path& path) noexcept;

	std::chrono::steady_clock::time_point m_nextScanTime{};
	std::unordered_map<std::string, std::filesystem::file_time_type> m_writeTimes;
	bool m_hasPrimedSnapshot = false;
};