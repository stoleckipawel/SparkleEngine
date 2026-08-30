#include "PCH.h"

#include "Core/Public/Hash/HashUtils.h"

#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>

#if defined(_WIN32)
  #define NOMINMAX
  #ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
  #endif
  #include <Windows.h>
  #include <bcrypt.h>
#endif

namespace Hash
{
	static constexpr std::size_t kHashBufferSize = 64 * 1024;

	uint64_t FinalizeFnv1a64(uint64_t hash) noexcept
	{
		return hash != 0 ? hash : kFnv64OffsetBasis;
	}

	uint64_t ContinueFnv1a64(uint64_t hash, const void* data, size_t size) noexcept
	{
		const auto* bytes = static_cast<const unsigned char*>(data);
		for (size_t index = 0; index < size; ++index)
		{
			hash ^= static_cast<uint64_t>(bytes[index]);
			hash *= kFnv64Prime;
		}
		return hash;
	}

	uint64_t Fnv1a64(std::string_view str) noexcept
	{
		uint64_t hash = kFnv64OffsetBasis;
		for (const char character : str)
		{
			hash ^= static_cast<uint64_t>(static_cast<unsigned char>(character));
			hash *= kFnv64Prime;
		}
		return hash;
	}

	uint64_t Fnv1a64(const void* data, size_t size) noexcept
	{
		return ContinueFnv1a64(kFnv64OffsetBasis, data, size);
	}

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

	bool TrySha256Hex(std::string_view text, std::string& outHashHex, std::string& outErrorMessage)
	{
		outHashHex.clear();
		outErrorMessage.clear();

#if defined(_WIN32)
		BCRYPT_ALG_HANDLE algorithmHandle = nullptr;
		if (BCryptOpenAlgorithmProvider(&algorithmHandle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
		{
			outErrorMessage = "Failed to open SHA-256 provider.";
			return false;
		}

		BCRYPT_HASH_HANDLE hashHandle = nullptr;
		std::array<unsigned char, 32> hash = {};
		if (BCryptCreateHash(algorithmHandle, &hashHandle, nullptr, 0, nullptr, 0, 0) != 0)
		{
			BCryptCloseAlgorithmProvider(algorithmHandle, 0);
			outErrorMessage = "Failed to create SHA-256 hash.";
			return false;
		}

		const bool hashed = BCryptHashData(
		                        hashHandle,
		                        reinterpret_cast<unsigned char*>(const_cast<char*>(text.data())),
		                        static_cast<unsigned long>(text.size()),
		                        0)
		        == 0
		    && BCryptFinishHash(hashHandle, hash.data(), static_cast<unsigned long>(hash.size()), 0) == 0;
		BCryptDestroyHash(hashHandle);
		BCryptCloseAlgorithmProvider(algorithmHandle, 0);
		if (!hashed)
		{
			outErrorMessage = "Failed to compute SHA-256 hash.";
			return false;
		}

		std::ostringstream stream;
		stream << std::hex << std::setfill('0');
		for (const unsigned char byte : hash)
		{
			stream << std::setw(2) << static_cast<int>(byte);
		}

		outHashHex = stream.str();
		return true;
#else
		(void) text;
		outErrorMessage = "SHA-256 hashing is not implemented for this platform.";
		return false;
#endif
	}

	uint32_t Fnv1a32(std::string_view str) noexcept
	{
		uint32_t hash = kFnv32OffsetBasis;
		for (const char character : str)
		{
			hash ^= static_cast<uint32_t>(static_cast<unsigned char>(character));
			hash *= kFnv32Prime;
		}
		return hash;
	}
}
