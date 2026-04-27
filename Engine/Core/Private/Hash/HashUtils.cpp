#include "PCH.h"

#include "Core/Public/Hash/HashUtils.h"

#include <array>
#include <fstream>

namespace Hash
{
	static constexpr std::size_t kHashBufferSize = 64 * 1024;

	bool TryFnv1a64File(const std::filesystem::path& path, uint64_t& outHash, std::string& outErrorMessage)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input.is_open())
		{
			outHash = 0;
			outErrorMessage = "Failed to open file for hashing: '" + path.string() + "'";
			return false;
		}

		uint64_t hash = kFnv64OffsetBasis;
		std::array<char, kHashBufferSize> buffer{};
		while (input.good())
		{
			input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			const std::streamsize bytesRead = input.gcount();
			hash = ContinueFnv1a64(hash, buffer.data(), static_cast<std::size_t>(bytesRead));
		}

		if (!input.eof())
		{
			outHash = 0;
			outErrorMessage = "Failed to read file for hashing: '" + path.string() + "'";
			return false;
		}

		outHash = FinalizeFnv1a64(hash);
		outErrorMessage.clear();
		return true;
	}
}