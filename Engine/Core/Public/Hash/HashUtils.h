#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Hash
{
	inline constexpr uint64_t kFnv64OffsetBasis = 14695981039346656037ull;
	inline constexpr uint64_t kFnv64Prime = 1099511628211ull;

	constexpr uint64_t FinalizeFnv1a64(uint64_t hash) noexcept
	{
		return hash != 0 ? hash : kFnv64OffsetBasis;
	}

	inline uint64_t ContinueFnv1a64(uint64_t hash, const void* data, size_t size) noexcept
	{
		const auto* bytes = static_cast<const unsigned char*>(data);
		for (size_t i = 0; i < size; ++i)
		{
			hash ^= static_cast<uint64_t>(bytes[i]);
			hash *= kFnv64Prime;
		}
		return hash;
	}

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

	constexpr uint64_t Fnv1a64(std::string_view str) noexcept
	{
		uint64_t hash = kFnv64OffsetBasis;
		for (const char character : str)
		{
			hash ^= static_cast<uint64_t>(static_cast<unsigned char>(character));
			hash *= kFnv64Prime;
		}
		return hash;
	}

	inline uint64_t Fnv1a64(const void* data, size_t size) noexcept
	{
		return ContinueFnv1a64(kFnv64OffsetBasis, data, size);
	}

	bool TryFnv1a64File(const std::filesystem::path& path, uint64_t& outHash, std::string& outErrorMessage);

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
