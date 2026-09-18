#pragma once

#include "Core/Public/CoreAPI.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Hash
{
	using Sha256Digest = std::array<std::byte, 32>;

	inline constexpr uint64_t kFnv64OffsetBasis = 14695981039346656037ull;
	inline constexpr uint64_t kFnv64Prime = 1099511628211ull;

	SPARKLE_CORE_API uint64_t FinalizeFnv1a64(uint64_t hash) noexcept;
	SPARKLE_CORE_API uint64_t ContinueFnv1a64(uint64_t hash, const void* data, size_t size) noexcept;

	template <typename TValue> uint64_t ContinueFnv1a64Value(uint64_t hash, const TValue& value) noexcept
	{
		static_assert(std::is_trivially_copyable_v<TValue>, "FNV value hashing requires trivially-copyable values");
		return ContinueFnv1a64(hash, &value, sizeof(TValue));
	}

	template <typename TValue> uint64_t ContinueFnv1a64Vector(uint64_t hash, const std::vector<TValue>& values) noexcept
	{
		static_assert(std::is_trivially_copyable_v<TValue>, "FNV vector hashing requires trivially-copyable values");
		if (values.empty())
		{
			return hash;
		}
		return ContinueFnv1a64(hash, values.data(), sizeof(TValue) * values.size());
	}

	SPARKLE_CORE_API uint64_t Fnv1a64(std::string_view str) noexcept;
	SPARKLE_CORE_API uint64_t Fnv1a64(const void* data, size_t size) noexcept;
	SPARKLE_CORE_API bool TryFnv1a64File(const std::filesystem::path& path, uint64_t& outHash, std::string& outErrorMessage);
	SPARKLE_CORE_API bool TrySha256(const void* data, size_t size, Sha256Digest& outHash, std::string& outErrorMessage);
	SPARKLE_CORE_API bool TrySha256File(const std::filesystem::path& path, Sha256Digest& outHash, std::string& outErrorMessage);
	SPARKLE_CORE_API std::string Sha256ToHex(const Sha256Digest& hash);
	SPARKLE_CORE_API bool TrySha256Hex(std::span<const std::byte> bytes, std::string& outHashHex, std::string& outErrorMessage);
	SPARKLE_CORE_API bool TrySha256Hex(std::string_view text, std::string& outHashHex, std::string& outErrorMessage);

	inline constexpr uint32_t kFnv32OffsetBasis = 2166136261u;
	inline constexpr uint32_t kFnv32Prime = 16777619u;

	SPARKLE_CORE_API uint32_t Fnv1a32(std::string_view str) noexcept;
}
