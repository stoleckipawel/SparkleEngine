// ============================================================================
// HashUtils.h
// FNV-1a 64-bit hash implementation for compile-time and runtime use.
// ----------------------------------------------------------------------------
// USAGE:
//   constexpr uint64_t hash = Engine::Hash::Fnv1a64("my_string");
//   uint64_t runtimeHash = Engine::Hash::Fnv1a64(data, size);
//
// DESIGN:
//   - FNV-1a chosen for excellent distribution and simplicity
//   - constexpr enables compile-time hash computation
//   - Non-cryptographic: suitable for hash tables, not security
// ============================================================================
#pragma once

#include <cstdint>
#include <string_view>

namespace Engine
{
	namespace Hash
	{
		// FNV-1a (Fowler-Noll-Vo) 64-bit hash constants.
		// Non-cryptographic hash with excellent distribution, suitable for hash tables.
		inline constexpr uint64_t kFnv64OffsetBasis = 14695981039346656037ull;
		inline constexpr uint64_t kFnv64Prime = 1099511628211ull;

		// Computes FNV-1a 64-bit hash of a string.
		// Works at both compile-time (constexpr) and runtime.
		constexpr uint64_t Fnv1a64(std::string_view str) noexcept
		{
			uint64_t hash = kFnv64OffsetBasis;
			for (const char c : str)
			{
				hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
				hash *= kFnv64Prime;
			}
			return hash;
		}

		// Computes FNV-1a 64-bit hash of raw bytes.
		constexpr uint64_t Fnv1a64(const void* data, size_t size) noexcept
		{
			uint64_t hash = kFnv64OffsetBasis;
			const auto* bytes = static_cast<const unsigned char*>(data);
			for (size_t i = 0; i < size; ++i)
			{
				hash ^= static_cast<uint64_t>(bytes[i]);
				hash *= kFnv64Prime;
			}
			return hash;
		}

		// FNV-1a 32-bit variant (for when 64-bit is overkill).
		inline constexpr uint32_t kFnv32OffsetBasis = 2166136261u;
		inline constexpr uint32_t kFnv32Prime = 16777619u;

		constexpr uint32_t Fnv1a32(std::string_view str) noexcept
		{
			uint32_t hash = kFnv32OffsetBasis;
			for (const char c : str)
			{
				hash ^= static_cast<uint32_t>(static_cast<unsigned char>(c));
				hash *= kFnv32Prime;
			}
			return hash;
		}
	}  // namespace Hash
}  // namespace Engine
