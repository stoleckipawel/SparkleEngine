#include "PCH.h"

#include "Assets/Loaders/CookedAssetFileSet.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Files/FileUtils.h"
#include "Level/Loading/SceneLoadBudget.h"

#include <format>

namespace Assets
{
	void CookedAssetFileSet::Read(const std::filesystem::path& path, SceneLoadBudget& budget)
	{
		if (m_files.contains(path))
			return;
		std::error_code sizeError;
		const std::uintmax_t fileSize = std::filesystem::file_size(path, sizeError);
		if (sizeError || fileSize > budget.GetMaximumBytes())
		{
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' could not be admitted within the scene-load raw-byte budget.", path.generic_string()));
		}

		const std::size_t admittedBytes = static_cast<std::size_t>(fileSize);
		if (!budget.TryReserve(admittedBytes))
		{
			throw Diagnostics::Error("Scene load exceeded the aggregate retained-data byte budget.");
		}

		std::size_t retainedReservation = admittedBytes;
		try
		{
			std::vector<std::uint8_t> bytes;
			std::string errorMessage;
			if (!Files::TryReadAllBytes(path, bytes, errorMessage))
			{
				throw Diagnostics::Error(std::format("Could not read cooked asset '{}': {}", path.generic_string(), errorMessage));
			}
			if (bytes.size() != fileSize)
			{
				throw Diagnostics::Error(
				    std::format("Cooked asset '{}' changed while the scene-load request was reading it.", path.generic_string()));
			}

			const std::size_t retainedBytes = bytes.capacity() * sizeof(std::uint8_t);
			if (retainedBytes > admittedBytes && !budget.TryReserve(retainedBytes - admittedBytes))
			{
				throw Diagnostics::Error("Scene load exceeded the aggregate retained-data byte budget.");
			}
			retainedReservation = retainedBytes;
			m_files.emplace(path, std::move(bytes));
			m_byteCount += retainedBytes;
		}
		catch (...)
		{
			budget.Release(retainedReservation);
			throw;
		}
	}

	std::span<const std::uint8_t> CookedAssetFileSet::Get(const std::filesystem::path& path) const
	{
		const auto file = m_files.find(path);
		if (file == m_files.end())
		{
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' was not retained by the scene-load transaction.", path.generic_string()));
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
