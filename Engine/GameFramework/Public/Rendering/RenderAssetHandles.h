#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <compare>
#include <cstdint>
#include <limits>
#include <memory>

class Mesh;

template <typename TDomain> class RenderAssetHandle final
{
  public:
	constexpr RenderAssetHandle() noexcept = default;
	constexpr explicit RenderAssetHandle(Assets::CookedAssetId assetId) noexcept : m_assetId(assetId) {}

	constexpr bool IsValid() const noexcept { return m_assetId != Assets::InvalidCookedAssetId; }
	constexpr Assets::CookedAssetId GetAssetId() const noexcept { return m_assetId; }
	constexpr auto operator<=>(const RenderAssetHandle&) const noexcept = default;

  private:
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
};

struct RenderSkeletonAssetDomain;
struct RenderAnimationAssetDomain;
using RenderSkeletonAssetHandle = RenderAssetHandle<RenderSkeletonAssetDomain>;
using RenderAnimationAssetHandle = RenderAssetHandle<RenderAnimationAssetDomain>;

struct RenderTextureAssetHandle final
{
	std::uint32_t Index = (std::numeric_limits<std::uint32_t>::max)();

	constexpr bool IsValid() const noexcept
	{
		return Index != (std::numeric_limits<std::uint32_t>::max)();
	}
	constexpr auto operator<=>(const RenderTextureAssetHandle&) const noexcept = default;
};

class SPARKLE_ENGINE_API ImmutableRenderMeshHandle final
{
  public:
	ImmutableRenderMeshHandle() noexcept = default;
	ImmutableRenderMeshHandle(
	    Assets::CookedAssetId assetId,
	    std::uint32_t generation,
	    std::shared_ptr<const Mesh> resource) noexcept :
	    m_assetId(assetId), m_generation(generation), m_resource(std::move(resource))
	{
	}
	bool IsValid() const noexcept
	{
		return m_assetId != Assets::InvalidCookedAssetId &&
		       m_generation != 0 &&
		       m_resource != nullptr;
	}
	Assets::CookedAssetId GetAssetId() const noexcept { return m_assetId; }
	std::uint32_t GetGeneration() const noexcept { return m_generation; }
	const std::shared_ptr<const Mesh>& GetResource() const noexcept { return m_resource; }
	bool RefersToSameResource(const ImmutableRenderMeshHandle& other) const noexcept
	{
		return m_assetId == other.m_assetId &&
		       m_generation == other.m_generation &&
		       m_resource == other.m_resource;
	}

  private:
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
	std::uint32_t m_generation = 0;
	std::shared_ptr<const Mesh> m_resource;
};
