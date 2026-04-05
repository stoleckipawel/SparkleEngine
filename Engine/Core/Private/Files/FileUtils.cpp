#include "PCH.h"

#include "Core/Public/Files/FileUtils.h"

#include <format>
#include <fstream>
#include <limits>

namespace Engine::Files
{
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
}