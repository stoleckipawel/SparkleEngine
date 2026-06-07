#include "PCH.h"

#include "Assets/SceneAssetPayload.h"

bool SceneAssetPayload::HasMeshes() const noexcept
{
	return (!staticMeshAssets.empty() && !staticMeshInstances.empty()) || (!skeletalMeshAssets.empty() && !skeletalMeshInstances.empty());
}

std::size_t SceneAssetPayload::GetMeshCount() const noexcept
{
	return staticMeshInstances.size() + skeletalMeshInstances.size();
}

std::size_t SceneAssetPayload::GetMaterialCount() const noexcept
{
	return materials.size();
}
