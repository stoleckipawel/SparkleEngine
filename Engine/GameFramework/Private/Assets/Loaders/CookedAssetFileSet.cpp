#include "PCH.h"

#include "Assets/Loaders/CookedAssetFileSet.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cerrno>
#include <format>
#include <limits>
#include <string>
#include <system_error>

#if !defined(_WIN32)
  #include <fcntl.h>
  #include <sys/mman.h>
  #include <sys/stat.h>
  #include <unistd.h>
#endif

namespace Assets
{
	struct CookedAssetFileSet::File final
	{
		File() = default;
		~File() noexcept
		{
#if defined(_WIN32)
			if (Data != nullptr)
				UnmapViewOfFile(Data);
#else
			if (Data != nullptr)
				munmap(Data, Size);
#endif
		}

		File(const File&) = delete;
		File& operator=(const File&) = delete;

		void* Data = nullptr;
		std::size_t Size = 0;
		std::size_t ReferenceCount = 1;
	};

	namespace CookedAssetFileSetDetail
	{
		std::string FormatSystemError(unsigned long error)
		{
			return std::system_category().message(static_cast<int>(error));
		}
	}

	std::unique_ptr<CookedAssetFileSet::File> CookedAssetFileSet::Map(const std::filesystem::path& path)
	{
		auto mappedFile = std::make_unique<File>();
#if defined(_WIN32)
		const HANDLE fileHandle = CreateFileW(
		    path.c_str(),
		    GENERIC_READ,
		    FILE_SHARE_READ,
		    nullptr,
		    OPEN_EXISTING,
		    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
		    nullptr);
		if (fileHandle == INVALID_HANDLE_VALUE)
		{
			throw Diagnostics::Error(
			    std::format(
			        "Could not open cooked asset '{}': {}",
			        path.generic_string(),
			        CookedAssetFileSetDetail::FormatSystemError(GetLastError())));
		}

		LARGE_INTEGER fileSize{};
		if (!GetFileSizeEx(fileHandle, &fileSize))
		{
			const unsigned long error = GetLastError();
			CloseHandle(fileHandle);
			throw Diagnostics::Error(
			    std::format(
			        "Could not determine the size of cooked asset '{}': {}",
			        path.generic_string(),
			        CookedAssetFileSetDetail::FormatSystemError(error)));
		}
		if (fileSize.QuadPart <= 0 || static_cast<unsigned long long>(fileSize.QuadPart) > (std::numeric_limits<std::size_t>::max)())
		{
			CloseHandle(fileHandle);
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' has an unsupported empty or oversized payload.", path.generic_string()));
		}

		const HANDLE mappingHandle = CreateFileMappingW(fileHandle, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (mappingHandle == nullptr)
		{
			const unsigned long error = GetLastError();
			CloseHandle(fileHandle);
			throw Diagnostics::Error(
			    std::format(
			        "Could not map cooked asset '{}': {}",
			        path.generic_string(),
			        CookedAssetFileSetDetail::FormatSystemError(error)));
		}

		void* const data = MapViewOfFile(mappingHandle, FILE_MAP_READ, 0, 0, 0);
		const unsigned long mapError = data == nullptr ? GetLastError() : ERROR_SUCCESS;
		CloseHandle(mappingHandle);
		CloseHandle(fileHandle);
		if (data == nullptr)
		{
			throw Diagnostics::Error(
			    std::format(
			        "Could not map cooked asset '{}': {}",
			        path.generic_string(),
			        CookedAssetFileSetDetail::FormatSystemError(mapError)));
		}

		mappedFile->Data = data;
		mappedFile->Size = static_cast<std::size_t>(fileSize.QuadPart);
		return mappedFile;
#else
		const int descriptor = open(path.c_str(), O_RDONLY);
		if (descriptor < 0)
		{
			throw Diagnostics::Error(
			    std::format("Could not open cooked asset '{}': {}", path.generic_string(), std::generic_category().message(errno)));
		}

		struct stat status{};
		if (fstat(descriptor, &status) != 0)
		{
			const int error = errno;
			close(descriptor);
			throw Diagnostics::Error(
			    std::format(
			        "Could not determine the size of cooked asset '{}': {}",
			        path.generic_string(),
			        std::generic_category().message(error)));
		}
		if (status.st_size <= 0 || static_cast<unsigned long long>(status.st_size) > (std::numeric_limits<std::size_t>::max)())
		{
			close(descriptor);
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' has an unsupported empty or oversized payload.", path.generic_string()));
		}

		const std::size_t fileSize = static_cast<std::size_t>(status.st_size);
		void* const data = mmap(nullptr, fileSize, PROT_READ, MAP_PRIVATE, descriptor, 0);
		const int mapError = data == MAP_FAILED ? errno : 0;
		close(descriptor);
		if (data == MAP_FAILED)
		{
			throw Diagnostics::Error(
			    std::format("Could not map cooked asset '{}': {}", path.generic_string(), std::generic_category().message(mapError)));
		}

		mappedFile->Data = data;
		mappedFile->Size = fileSize;
		return mappedFile;
#endif
	}

	CookedAssetFileSet::CookedAssetFileSet() = default;
	CookedAssetFileSet::~CookedAssetFileSet() = default;
	CookedAssetFileSet::CookedAssetFileSet(CookedAssetFileSet&&) noexcept = default;
	CookedAssetFileSet& CookedAssetFileSet::operator=(CookedAssetFileSet&&) noexcept = default;

	void CookedAssetFileSet::Read(const std::filesystem::path& path)
	{
		const auto existing = m_files.find(path);
		if (existing != m_files.end())
		{
			if (existing->second->ReferenceCount == (std::numeric_limits<std::size_t>::max)())
				throw Diagnostics::Error(std::format("Cooked asset '{}' reference count exceeds the host range.", path.generic_string()));
			++existing->second->ReferenceCount;
			return;
		}
		m_files.emplace(path, Map(path));
	}

	std::span<const std::uint8_t> CookedAssetFileSet::Get(const std::filesystem::path& path) const
	{
		const auto file = m_files.find(path);
		if (file == m_files.end())
		{
			throw Diagnostics::Error(
			    std::format("Cooked asset '{}' was not retained by the scene-load transaction.", path.generic_string()));
		}
		return {static_cast<const std::uint8_t*>(file->second->Data), file->second->Size};
	}

	void CookedAssetFileSet::Release(const std::filesystem::path& path)
	{
		const auto file = m_files.find(path);
		if (file == m_files.end())
		{
			throw Diagnostics::Error(std::format("Cooked asset '{}' was released without a retained reference.", path.generic_string()));
		}
		if (--file->second->ReferenceCount == 0)
			m_files.erase(file);
	}

	void CookedAssetFileSet::Reset() noexcept
	{
		m_files.clear();
	}
}
