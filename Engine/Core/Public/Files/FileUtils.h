#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <vector>

namespace Files
{
	bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage);
	bool TryWriteAllBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes, std::string& outErrorMessage);
	bool TryWriteAllText(const std::filesystem::path& path, std::string_view text, std::string& outErrorMessage);
	bool TryWriteAllTextAtomic(const std::filesystem::path& path, std::string_view text, std::string& outErrorMessage);
	bool TryOpenBinaryOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage);
	bool TryOpenTextOutput(const std::filesystem::path& path, std::ofstream& output, std::string& outErrorMessage);
	std::filesystem::path BuildTemporaryPath(const std::filesystem::path& path, std::string_view suffix = ".tmp");
	bool TryFinalizeTemporaryFile(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage);
	bool TryFinalizeTemporaryFileIfMissing(
	    const std::filesystem::path& temporaryPath,
	    const std::filesystem::path& finalPath,
	    std::string& outErrorMessage);
	void CleanupTemporaryFile(const std::filesystem::path& temporaryPath, std::ofstream* output = nullptr) noexcept;
	bool TryCloseOutput(std::ofstream& output, const std::filesystem::path& path, std::string& outErrorMessage);
}