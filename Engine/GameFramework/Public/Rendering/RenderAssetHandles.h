#pragma once

#include "GameFramework/Public/Assets/Cooked/CookedAssetCommon.h"
#include "GameFramework/Public/GameFrameworkAPI.h"

#include <cstdint>
#include <memory>

class Mesh;

class SPARKLE_ENGINE_API ImmutableRenderMeshHandle final
{
  public:
	ImmutableRenderMeshHandle() noexcept = default;
	ImmutableRenderMeshHandle(
	    Assets::CookedAssetId assetId, std::uint32_t version, std::shared_ptr<const Mesh> resource) noexcept :
	    m_assetId(assetId), m_version(version), m_resource(std::move(resource))
	{
	}
	bool IsValid() const noexcept { return m_resource != nullptr && m_version != 0; }
	Assets::CookedAssetId GetAssetId() const noexcept { return m_assetId; }
	std::uint32_t GetVersion() const noexcept { return m_version; }
	const std::shared_ptr<const Mesh>& GetResource() const noexcept { return m_resource; }

  private:
	Assets::CookedAssetId m_assetId = Assets::InvalidCookedAssetId;
	std::uint32_t m_version = 0;
	std::shared_ptr<const Mesh> m_resource;
};
