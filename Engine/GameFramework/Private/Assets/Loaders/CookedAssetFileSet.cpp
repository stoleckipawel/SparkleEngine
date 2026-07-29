#include "PCH.h"

#include "Assets/Loaders/CookedAssetFileSet.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"

#include <format>

namespace Assets
{
	void CookedAssetFileSet::Read(
	    const std::filesystem::path& path,
	    std::atomic<std::size_t>& retainedBytes,
	    std::size_t maximumBytes)
	{
		if (m_files.contains(path))
			return;
		std::error_code sizeError;
		const std::uintmax_t fileSize = std::filesystem::file_size(path, sizeError);
		if (sizeError || fileSize > maximumBytes)
		{
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' could not be admitted within the scene-load byte budget.", path.generic_string()));
		}
		std::size_t current = retainedBytes.load(std::memory_order_relaxed);
		while (current <= maximumBytes - static_cast<std::size_t>(fileSize) &&
		       !retainedBytes.compare_exchange_weak(
		           current, current + static_cast<std::size_t>(fileSize), std::memory_order_acq_rel, std::memory_order_relaxed))
		{
		}
		if (current > maximumBytes - static_cast<std::size_t>(fileSize))
		{
			throw Diagnostics::Error("Scene load exceeded the retained raw-data byte budget.");
		}
		std::vector<std::uint8_t> bytes;
		std::string errorMessage;
		if (!Files::TryReadAllBytes(path, bytes, errorMessage))
		{
			retainedBytes.fetch_sub(static_cast<std::size_t>(fileSize), std::memory_order_acq_rel);
			throw Diagnostics::Error(std::format("Could not read cooked asset '{}': {}", path.generic_string(), errorMessage));
		}
		if (bytes.size() != fileSize)
		{
			retainedBytes.fetch_sub(static_cast<std::size_t>(fileSize), std::memory_order_acq_rel);
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' changed while the scene-load request was reading it.", path.generic_string()));
		}
		m_byteCount += bytes.size();
		m_files.emplace(path, std::move(bytes));
	}

	std::span<const std::uint8_t> CookedAssetFileSet::Get(const std::filesystem::path& path) const
	{
		const auto file = m_files.find(path);
		if (file == m_files.end())
		{
			throw Diagnostics::Error(std::format("Cooked asset '{}' was not retained by the scene-load transaction.", path.generic_string()));
		}
		return std::span<const std::uint8_t>(file->second);
	}

	std::size_t CookedAssetFileSet::Reset() noexcept
	{
		const std::size_t releasedBytes = m_byteCount;
		m_files.clear();
		m_byteCount = 0;
		return releasedBytes;
	}
}
