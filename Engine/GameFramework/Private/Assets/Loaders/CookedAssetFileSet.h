#pragma once

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <span>

namespace Assets
{
	class CookedAssetFileSet final
	{
	public:
		CookedAssetFileSet();
		~CookedAssetFileSet();

		CookedAssetFileSet(const CookedAssetFileSet&) = delete;
		CookedAssetFileSet& operator=(const CookedAssetFileSet&) = delete;
		CookedAssetFileSet(CookedAssetFileSet&&) noexcept;
		CookedAssetFileSet& operator=(CookedAssetFileSet&&) noexcept;

		// Get spans remain valid until the matching manifest references are released or the set is reset.
		void Read(const std::filesystem::path& path);
		std::span<const std::uint8_t> Get(const std::filesystem::path& path) const;
		void Release(const std::filesystem::path& path);
		void Reset() noexcept;

	private:
		struct File;
		static std::unique_ptr<File> Map(const std::filesystem::path& path);
		std::map<std::filesystem::path, std::unique_ptr<File>> m_files;
	};
}
