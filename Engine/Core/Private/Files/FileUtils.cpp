#include "PCH.h"

#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <fstream>
#include <limits>

namespace Files
{
	static bool TryOpenOutput(
	    const std::filesystem::path& path,
	    const std::ios::openmode mode,
	    std::ofstream& output,
	    std::string& outErrorMessage)
	{
		std::error_code ec;
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
			{
				outErrorMessage = std::format("Failed to create directories for '{}': {}", path.string(), ec.message());
				return false;
			}
		}

		output.open(path, mode | std::ios::trunc);
		if (!output)
		{
			outErrorMessage = std::format("Failed to open '{}' for writing", path.string());
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage)
	{
		std::ifstream input(path, std::ios::binary | std::ios::ate);
		if (!input)
		{
			outErrorMessage = std::format("Failed to open '{}'", path.string());
			return false;
		}

		const std::ifstream::pos_type fileSize = input.tellg();
		if (fileSize <= 0)
		{
			outErrorMessage = std::format("'{}' is empty", path.string());
			return false;
		}

		if (static_cast<std::uint64_t>(fileSize) > (std::numeric_limits<std::size_t>::max)())
		{
			outErrorMessage = std::format("'{}' is too large to load", path.string());
			return false;
		}

		outBytes.resize(static_cast<std::size_t>(fileSize));
		input.seekg(0, std::ios::beg);
		if (!input.read(reinterpret_cast<char*>(outBytes.data()), fileSize))
		{
			outErrorMessage = std::format("Failed to read '{}'", path.string());
			outBytes.clear();
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TryWriteAllBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes, std::string& outErrorMessage)
	{
		std::error_code ec;
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
			{
				outErrorMessage = std::format("Failed to create directories for '{}': {}", path.string(), ec.message());
				return false;
			}
		}

		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		if (!output)
		{
			outErrorMessage = std::format("Failed to open '{}' for writing", path.string());
			return false;
		}

		if (!bytes.empty() && !output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size())))
		{
			outErrorMessage = std::format("Failed to write '{}'", path.string());
			return false;
		}

		output.flush();
		if (!output)
		{
			outErrorMessage = std::format("Failed to flush '{}'", path.string());
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TryWriteAllText(const std::filesystem::path& path, std::string_view text, std::string& outErrorMessage)
	{
		std::error_code ec;
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
			{
				outErrorMessage = std::format("Failed to create directories for '{}': {}", path.string(), ec.message());
				return false;
			}
		}

		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		if (!output)
		{
			outErrorMessage = std::format("Failed to open '{}' for writing", path.string());
			return false;
		}

		if (!text.empty() && !output.write(text.data(), static_cast<std::streamsize>(text.size())))
		{
			outErrorMessage = std::format("Failed to write '{}'", path.string());
			return false;
		}

		output.flush();
		if (!output)
		{
			outErrorMessage = std::format("Failed to flush '{}'", path.string());
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TryWriteAllTextAtomic(const std::filesystem::path& path, std::string_view text, std::string& outErrorMessage)
	{
		const std::filesystem::path temporaryPath = BuildTemporaryPath(path);

		std::error_code ec;
		std::filesystem::remove(temporaryPath, ec);

		if (!TryWriteAllText(temporaryPath, text, outErrorMessage))
		{
			return false;
		}

		if (!TryFinalizeTemporaryFile(temporaryPath, path, outErrorMessage))
		{
			std::filesystem::remove(temporaryPath, ec);
			return false;
		}

		outErrorMessage.clear();
		return true;
	}

	bool TryOpenBinaryOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage)
	{
		return TryOpenOutput(path, std::ios::binary, output, outErrorMessage);
	}

	bool TryOpenTextOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage)
	{
		return TryOpenOutput(path, static_cast<std::ios::openmode>(0), output, outErrorMessage);
	}

	std::filesystem::path BuildTemporaryPath(const std::filesystem::path& path, std::string_view suffix)
	{
		std::filesystem::path temporaryPath = path;
		temporaryPath += std::string(suffix.empty() ? std::string_view{".tmp"} : suffix);
		return temporaryPath;
	}

	bool TryFinalizeTemporaryFile(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage)
	{
		std::error_code ec;
		std::filesystem::rename(temporaryPath, finalPath, ec);
		if (!ec)
		{
			outErrorMessage.clear();
			return true;
		}

		if (std::filesystem::exists(finalPath))
		{
			ec.clear();
			std::filesystem::remove(finalPath, ec);
			if (ec)
			{
				outErrorMessage = std::format(
				    "Failed to replace existing output '{}'. The destination file may still be in use.",
				    finalPath.string());
				return false;
			}

			ec.clear();
			std::filesystem::rename(temporaryPath, finalPath, ec);
			if (!ec)
			{
				outErrorMessage.clear();
				return true;
			}
		}

		outErrorMessage = std::format("Failed to move temporary file '{}' into place as '{}'", temporaryPath.string(), finalPath.string());
		return false;
	}

	bool TryFinalizeTemporaryFileIfMissing(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage)
	{
		std::error_code ec;
		if (std::filesystem::exists(finalPath, ec) && !ec)
		{
			CleanupTemporaryFile(temporaryPath);
			outErrorMessage.clear();
			return true;
		}

		ec.clear();
		std::filesystem::rename(temporaryPath, finalPath, ec);
		if (!ec)
		{
			outErrorMessage.clear();
			return true;
		}

		std::error_code existsError;
		if (std::filesystem::exists(finalPath, existsError) && !existsError)
		{
			CleanupTemporaryFile(temporaryPath);
			outErrorMessage.clear();
			return true;
		}

		CleanupTemporaryFile(temporaryPath);
		outErrorMessage = std::format("Failed to move temporary file '{}' into place as '{}'", temporaryPath.string(), finalPath.string());
		return false;
	}

	void CleanupTemporaryFile(const std::filesystem::path& temporaryPath, std::ofstream* output) noexcept
	{
		if (output != nullptr && output->is_open())
		{
			output->close();
		}

		std::error_code ec;
		std::filesystem::remove(temporaryPath, ec);
	}

	bool TryCloseOutput(std::ofstream& output, const std::filesystem::path& path, std::string& outErrorMessage)
	{
		output.close();
		if (!output.fail())
		{
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage = std::format("Failed to finalize output '{}'", path.string());
		return false;
	}
}