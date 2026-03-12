#pragma once

#include <cstdint>
#include <string_view>

namespace Engine
{
	namespace Hash
	{
		inline constexpr uint64_t kFnv64OffsetBasis = 14695981039346656037ull;
		inline constexpr uint64_t kFnv64Prime = 1099511628211ull;

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
	}
}
