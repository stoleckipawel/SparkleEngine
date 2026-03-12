// ============================================================================
// AssetId.h
// Compile-time and runtime asset identification using FNV-1a 64-bit hashes.
// ----------------------------------------------------------------------------
// USAGE:
//   // Compile-time (zero runtime cost):
//   constexpr AssetId diffuseId = "textures/brick_diffuse.png"_asset;
//
//   // Runtime:
//   AssetId dynamicId(userProvidedPath);
//
//   // As map key:
//   std::unordered_map<AssetId, TextureHandle> textureCache;
//
// DESIGN:
//   - 8-byte hash provides O(1) lookups instead of string comparisons
//   - constexpr construction enables compile-time hash computation
//   - Debug builds store original path string to detect collisions
//
// NOTES:
//   - FNV-1a 64-bit has ~1 in 10^14 collision probability for <100k assets
//   - Use with asset registries, caches, hot-reload, dependency tracking
// ============================================================================
#pragma once

#include "Hash/HashUtils.h"

#include <cstdint>
#include <functional>
#include <string_view>

// Strongly-typed 64-bit asset identifier.
// Immutable after construction. Trivially copyable. Safe to pass by value.
class AssetId final
{
  public:
	// Default constructs an invalid (zero) ID.
	constexpr AssetId() noexcept = default;

	// Constructs from asset path/name. Hash computed at compile-time if constexpr.
	constexpr explicit AssetId(std::string_view name) noexcept :
	    m_hash(Engine::Hash::Fnv1a64(name))
#if defined(_DEBUG)
	    ,
	    m_debugName(name)
#endif
	{
	}

	// Returns the underlying 64-bit hash value.
	// Use this for serialization, switch statements, or when raw hash is needed.
	constexpr uint64_t GetHash() const noexcept { return m_hash; }

	// Returns true if this ID represents a valid asset (non-zero hash).
	// An empty string hashes to non-zero, so only default-constructed IDs are invalid.
	constexpr bool IsValid() const noexcept { return m_hash != 0; }

	// Enables `if (assetId)` syntax for validity checks.
	constexpr explicit operator bool() const noexcept { return IsValid(); }

#if defined(_DEBUG)
	// Returns the original string used to create this ID (debug builds only).
	// Useful for logging, debugging, and collision detection.
	constexpr std::string_view GetDebugName() const noexcept { return m_debugName; }
#endif

	// Equality comparison. Two AssetIds are equal if their hashes match.
	// Using `= default` generates optimal code and enables constexpr comparison.
	constexpr bool operator==(const AssetId& other) const noexcept = default;

	// Three-way comparison for ordered containers (std::map, std::set).
	// Using `= default` generates optimal code comparing m_hash directly.
	constexpr auto operator<=>(const AssetId& other) const noexcept = default;

  private:
	uint64_t m_hash = 0;

#if defined(_DEBUG)
	// Stored only in debug to help identify collisions and aid debugging.
	// In release, AssetId is exactly 8 bytes.
	std::string_view m_debugName;
#endif
};

// std::hash specialization enables AssetId as key in std::unordered_map/set.
template <> struct std::hash<AssetId>
{
	constexpr size_t operator()(const AssetId& id) const noexcept { return static_cast<size_t>(id.GetHash()); }
};

// =============================================================================
// User-Defined Literal: _asset
// =============================================================================
//
// Allows writing asset IDs as string literals with the _asset suffix.
// The hash is computed at compile time (consteval), so there's zero runtime cost.
//
// EXAMPLES:
//   constexpr auto id = "textures/diffuse.png"_asset;  // Compile-time
//   auto id2 = "shaders/pbr.hlsl"_asset;               // Also compile-time
//
// NOTE: This only works with string literals. For runtime strings, use the
//       AssetId(std::string_view) constructor directly.
//
consteval AssetId operator""_asset(const char* str, size_t len) noexcept
{
	return AssetId(std::string_view(str, len));
}
