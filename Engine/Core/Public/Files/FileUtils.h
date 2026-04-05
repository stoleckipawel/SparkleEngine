#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace Engine::Files
{
	bool TryReadAllBytes(const std::filesystem::path& path, std::vector<std::uint8_t>& outBytes, std::string& outErrorMessage);
}