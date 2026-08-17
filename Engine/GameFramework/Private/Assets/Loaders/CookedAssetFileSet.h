#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <vector>

namespace Assets
{
	class SceneLoadBudget;

	class CookedAssetFileSet final
	{
	public:
		void Read(const std::filesystem::path& path, SceneLoadBudget& budget);
		std::span<const std::uint8_t> Get(const std::filesystem::path& path) const;
		std::size_t GetByteCount() const noexcept { return m_byteCount; }
		std::size_t Reset() noexcept;

	private:
		std::map<std::filesystem::path, std::vector<std::uint8_t>> m_files;
		std::size_t m_byteCount = 0;
	};
}
