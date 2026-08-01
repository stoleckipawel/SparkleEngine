#include "PCH.h"

#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <fstream>

namespace Files
{
	static bool TryOpenOutput(
	    const std::filesystem::path& path,
	    std::ios::openmode mode,
	    std::ofstream& output,
	    std::string& outErrorMessage)
	{
		std::error_code errorCode;
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path(), errorCode);
			if (errorCode)
			{
				outErrorMessage = std::format("Failed to create directories for '{}': {}", path.string(), errorCode.message());
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

	bool TryWriteAllBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes, std::string& outErrorMessage)
	{
		std::ofstream output;
		if (!TryOpenBinaryOutput(path, output, outErrorMessage))
		{
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
		std::ofstream output;
		if (!TryOpenBinaryOutput(path, output, outErrorMessage))
		{
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
		CleanupTemporaryFile(temporaryPath);

		if (!TryWriteAllText(temporaryPath, text, outErrorMessage))
		{
			return false;
		}
		if (!TryFinalizeTemporaryFile(temporaryPath, path, outErrorMessage))
		{
			CleanupTemporaryFile(temporaryPath);
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
