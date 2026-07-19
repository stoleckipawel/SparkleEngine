#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>
#include <atomic>

namespace Assets
{
	class CookedAssetFileSet final
	{
	  public:
		bool Read(
		    const std::filesystem::path& path,
		    std::atomic<std::size_t>& retainedBytes,
		    std::size_t maximumBytes,
		    std::string& errorMessage);
		std::span<const std::uint8_t> Find(const std::filesystem::path& path) const noexcept;
		std::size_t GetByteCount() const noexcept { return m_byteCount; }
		std::size_t Reset() noexcept;

	  private:
		std::map<std::filesystem::path, std::vector<std::uint8_t>> m_files;
		std::size_t m_byteCount = 0;
	};
}
