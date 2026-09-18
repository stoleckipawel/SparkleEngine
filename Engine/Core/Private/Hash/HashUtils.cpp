#include "PCH.h"

#include "Core/Public/Hash/HashUtils.h"

#include <algorithm>
#include <array>
#include <fstream>
#include <iomanip>
#include <limits>
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

#if defined(_WIN32)
	class Sha256Context final
	{
	public:
		~Sha256Context()
		{
			if (m_hash != nullptr)
			{
				BCryptDestroyHash(m_hash);
			}
			if (m_algorithm != nullptr)
			{
				BCryptCloseAlgorithmProvider(m_algorithm, 0);
			}
		}

		bool Initialize(std::string& errorMessage) noexcept
		{
			if (BCryptOpenAlgorithmProvider(&m_algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != 0)
			{
				errorMessage = "Failed to open SHA-256 provider.";
				return false;
			}
			if (BCryptCreateHash(m_algorithm, &m_hash, nullptr, 0, nullptr, 0, 0) != 0)
			{
				errorMessage = "Failed to create SHA-256 hash.";
				return false;
			}
			return true;
		}

		bool Update(const void* data, std::size_t size, std::string& errorMessage) noexcept
		{
			const auto* bytes = static_cast<const unsigned char*>(data);
			while (size > 0)
			{
				const std::size_t chunk = (std::min) (size, static_cast<std::size_t>((std::numeric_limits<unsigned long>::max)()));
				if (BCryptHashData(m_hash, const_cast<unsigned char*>(bytes), static_cast<unsigned long>(chunk), 0) != 0)
				{
					errorMessage = "Failed to update SHA-256 hash.";
					return false;
				}
				bytes += chunk;
				size -= chunk;
			}
			return true;
		}

		bool Finish(Sha256Digest& hash, std::string& errorMessage) noexcept
		{
			if (BCryptFinishHash(m_hash, reinterpret_cast<unsigned char*>(hash.data()), static_cast<unsigned long>(hash.size()), 0) != 0)
			{
				errorMessage = "Failed to finish SHA-256 hash.";
				return false;
			}
			return true;
		}

	private:
		BCRYPT_ALG_HANDLE m_algorithm = nullptr;
		BCRYPT_HASH_HANDLE m_hash = nullptr;
	};
#endif

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

	bool TrySha256(const void* data, size_t size, Sha256Digest& outHash, std::string& outErrorMessage)
	{
		outHash = {};
		outErrorMessage.clear();

#if defined(_WIN32)
		Sha256Context context;
		return context.Initialize(outErrorMessage) && context.Update(data, size, outErrorMessage)
		    && context.Finish(outHash, outErrorMessage);
#else
		(void) data;
		(void) size;
		outErrorMessage = "SHA-256 hashing is not implemented for this platform.";
		return false;
#endif
	}

	bool TrySha256File(const std::filesystem::path& path, Sha256Digest& outHash, std::string& outErrorMessage)
	{
		outHash = {};
		outErrorMessage.clear();
#if defined(_WIN32)
		std::ifstream input(path, std::ios::binary);
		if (!input.is_open())
		{
			outErrorMessage = "Failed to open file for hashing: '" + path.string() + "'";
			return false;
		}

		Sha256Context context;
		if (!context.Initialize(outErrorMessage))
		{
			return false;
		}
		std::array<char, kHashBufferSize> buffer{};
		while (input.good())
		{
			input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
			const std::streamsize bytesRead = input.gcount();
			if (bytesRead > 0 && !context.Update(buffer.data(), static_cast<std::size_t>(bytesRead), outErrorMessage))
			{
				return false;
			}
		}
		if (!input.eof())
		{
			outErrorMessage = "Failed to read file for hashing: '" + path.string() + "'";
			return false;
		}
		return context.Finish(outHash, outErrorMessage);
#else
		(void) path;
		outErrorMessage = "SHA-256 hashing is not implemented for this platform.";
		return false;
#endif
	}

	std::string Sha256ToHex(const Sha256Digest& hash)
	{
		std::ostringstream stream;
		stream << std::hex << std::setfill('0');
		for (const auto byte : hash)
		{
			stream << std::setw(2) << static_cast<unsigned int>(byte);
		}
		return stream.str();
	}

	bool TrySha256Hex(std::span<const std::byte> bytes, std::string& outHashHex, std::string& outErrorMessage)
	{
		Sha256Digest hash{};
		if (!TrySha256(bytes.data(), bytes.size(), hash, outErrorMessage))
		{
			outHashHex.clear();
			return false;
		}
		outHashHex = Sha256ToHex(hash);
		return true;
	}

	bool TrySha256Hex(std::string_view text, std::string& outHashHex, std::string& outErrorMessage)
	{
		return TrySha256Hex(std::as_bytes(std::span<const char>(text.data(), text.size())), outHashHex, outErrorMessage);
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
