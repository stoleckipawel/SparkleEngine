#include "PCH.h"

#include "Scene/Meshes/SkeletalMeshComponent.h"

#include "Scene/Meshes/Mesh.h"

SkeletalMeshComponent::SkeletalMeshComponent(
    std::unique_ptr<Mesh>&& mesh,
    const Transform& transform,
    MaterialHandle materialHandle,
    Assets::CookedAssetId meshAssetId,
    SceneMeshAssetIndex meshAssetIndex,
    Assets::CookedAssetId skeletonAssetId,
    std::uint32_t sourceNodeIndex) noexcept :
    MeshComponent(
        std::move(mesh),
        SceneMeshKind::Skeletal,
        transform,
        materialHandle,
        meshAssetId,
        meshAssetIndex,
        kInvalidSceneMeshInstanceGroupIndex,
        skeletonAssetId,
        sourceNodeIndex)
{
}
