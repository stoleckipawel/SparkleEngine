#include "PCH.h"

#include "Assets/SceneAssetPayload.h"

bool SceneAssetPayload::HasMeshes() const noexcept
{
	return !meshInstances.empty();
}

std::size_t SceneAssetPayload::GetMeshCount() const noexcept
{
	return meshInstances.size();
}

std::size_t SceneAssetPayload::GetMaterialCount() const noexcept
{
	return materials.size();
}