#include "PCH.h"

#include "GameFramework/Public/Rendering/RenderAssetHandles.h"

#include <utility>

ImmutableRenderMeshHandle::ImmutableRenderMeshHandle(
    Assets::CookedAssetId assetId,
    std::uint32_t generation,
    std::shared_ptr<const Mesh> resource) noexcept :
    m_assetId(assetId), m_generation(generation), m_resource(std::move(resource))
{
}

bool ImmutableRenderMeshHandle::IsValid() const noexcept
{
	return m_assetId != Assets::InvalidCookedAssetId && m_generation != 0 && m_resource != nullptr;
}

bool ImmutableRenderMeshHandle::RefersToSameResource(const ImmutableRenderMeshHandle& other) const noexcept
{
	return m_assetId == other.m_assetId && m_generation == other.m_generation && m_resource == other.m_resource;
}
