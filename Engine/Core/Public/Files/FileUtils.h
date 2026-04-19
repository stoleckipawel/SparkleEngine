#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Engine::Files
{
	bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage);
	bool TryWriteAllBytes(const std::filesystem::path& path, const std::vector<std::uint8_t>& bytes, std::string& outErrorMessage);
	bool TryWriteAllText(const std::filesystem::path& path, std::string_view text, std::string& outErrorMessage);
}