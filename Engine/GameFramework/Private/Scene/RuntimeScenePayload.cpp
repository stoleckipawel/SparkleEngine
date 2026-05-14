#include "PCH.h"

#include "Scene/RuntimeScenePayload.h"

bool RuntimeScenePayload::HasMeshes() const noexcept
{
	return !meshInstances.empty();
}

std::size_t RuntimeScenePayload::GetMeshCount() const noexcept
{
	return meshInstances.size();
}

std::size_t RuntimeScenePayload::GetMaterialCount() const noexcept
{
	return materials.size();
}