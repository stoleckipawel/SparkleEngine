#include "PCH.h"

#include "Assets/Loaders/CookedAssetFileSet.h"

#include "Core/Public/Files/FileUtils.h"

namespace Assets
{
	bool CookedAssetFileSet::Read(
	    const std::filesystem::path& path,
	    std::atomic<std::size_t>& retainedBytes,
	    std::size_t maximumBytes,
	    std::string& errorMessage)
	{
		if (m_files.contains(path))
			return true;
		std::error_code sizeError;
		const std::uintmax_t fileSize = std::filesystem::file_size(path, sizeError);
		if (sizeError || fileSize > maximumBytes)
		{
			errorMessage = "Cooked asset file size could not be admitted within the scene-load byte budget.";
			return false;
		}
		std::size_t current = retainedBytes.load(std::memory_order_relaxed);
		while (current <= maximumBytes - static_cast<std::size_t>(fileSize) &&
		       !retainedBytes.compare_exchange_weak(
		           current, current + static_cast<std::size_t>(fileSize), std::memory_order_acq_rel, std::memory_order_relaxed))
		{
		}
		if (current > maximumBytes - static_cast<std::size_t>(fileSize))
		{
			errorMessage = "Scene load exceeded the retained raw-data byte budget.";
			return false;
		}
		std::vector<std::uint8_t> bytes;
		if (!Files::TryReadAllBytes(path, bytes, errorMessage))
		{
			retainedBytes.fetch_sub(static_cast<std::size_t>(fileSize), std::memory_order_acq_rel);
			return false;
		}
		if (bytes.size() != fileSize)
		{
			retainedBytes.fetch_sub(static_cast<std::size_t>(fileSize), std::memory_order_acq_rel);
			errorMessage = "Cooked asset file changed while the scene-load request was reading it.";
			return false;
		}
		m_byteCount += bytes.size();
		m_files.emplace(path, std::move(bytes));
		return true;
	}

	std::span<const std::uint8_t> CookedAssetFileSet::Find(const std::filesystem::path& path) const noexcept
	{
		const auto file = m_files.find(path);
		return file == m_files.end() ? std::span<const std::uint8_t>{} : std::span<const std::uint8_t>(file->second);
	}

	std::size_t CookedAssetFileSet::Reset() noexcept
	{
		const std::size_t releasedBytes = m_byteCount;
		m_files.clear();
		m_byteCount = 0;
		return releasedBytes;
	}
}
