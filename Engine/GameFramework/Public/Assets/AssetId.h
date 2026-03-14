#pragma once

#include "Hash/HashUtils.h"

#include <cstdint>
#include <functional>
#include <string_view>

class AssetId final
{
  public:
	constexpr AssetId() noexcept = default;

	constexpr explicit AssetId(std::string_view name) noexcept :
	    m_hash(Hash::Fnv1a64(name))
#if defined(_DEBUG)
	    ,
	    m_debugName(name)
#endif
	{
	}

	constexpr uint64_t GetHash() const noexcept { return m_hash; }

	constexpr bool IsValid() const noexcept { return m_hash != 0; }

	constexpr explicit operator bool() const noexcept { return IsValid(); }

#if defined(_DEBUG)

	constexpr std::string_view GetDebugName() const noexcept { return m_debugName; }
#endif

	constexpr bool operator==(const AssetId& other) const noexcept = default;

	constexpr auto operator<=>(const AssetId& other) const noexcept = default;

  private:
	uint64_t m_hash = 0;

#if defined(_DEBUG)

	std::string_view m_debugName;
#endif
};

template <> struct std::hash<AssetId>
{
	constexpr size_t operator()(const AssetId& id) const noexcept { return static_cast<size_t>(id.GetHash()); }
};

consteval AssetId operator""_asset(const char* str, size_t len) noexcept
{
	return AssetId(std::string_view(str, len));
}
