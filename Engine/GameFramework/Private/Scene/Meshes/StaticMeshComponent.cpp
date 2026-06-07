#include "PCH.h"

#include "Scene/Meshes/StaticMeshComponent.h"

#include "Scene/Meshes/Mesh.h"

StaticMeshComponent::StaticMeshComponent(
    std::unique_ptr<Mesh>&& mesh,
    const Transform& transform,
    MaterialHandle materialHandle,
    Assets::CookedAssetId meshAssetId,
    SceneMeshAssetIndex meshAssetIndex,
    SceneMeshInstanceGroupIndex meshInstanceGroupIndex,
    std::uint32_t sourceNodeIndex) noexcept :
    MeshComponent(
        std::move(mesh),
        SceneMeshKind::Static,
        transform,
        materialHandle,
        meshAssetId,
        meshAssetIndex,
        meshInstanceGroupIndex,
        Assets::InvalidCookedAssetId,
        sourceNodeIndex)
{
}
