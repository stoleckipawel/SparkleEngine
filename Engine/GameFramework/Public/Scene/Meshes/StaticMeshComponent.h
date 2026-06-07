#pragma once

#include "GameFramework/Public/GameFrameworkAPI.h"
#include "GameFramework/Public/Scene/Meshes/MeshComponent.h"

#include <memory>

class Mesh;

class SPARKLE_ENGINE_API StaticMeshComponent final : public MeshComponent
{
  public:
	StaticMeshComponent(
	    std::unique_ptr<Mesh>&& mesh,
	    const Transform& transform,
	    MaterialHandle materialHandle,
	    Assets::CookedAssetId meshAssetId,
	    SceneMeshAssetIndex meshAssetIndex,
	    SceneMeshInstanceGroupIndex meshInstanceGroupIndex,
	    std::uint32_t sourceNodeIndex = Assets::kInvalidCookedSceneSourceNodeIndex) noexcept;
};
