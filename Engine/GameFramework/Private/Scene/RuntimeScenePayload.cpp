#include "PCH.h"

#include "Scene/RuntimeScenePayload.h"

bool RuntimeScenePayload::HasMeshes() const noexcept
{
	return !meshes.empty();
}

std::size_t RuntimeScenePayload::GetMeshCount() const noexcept
{
	return meshes.size();
}

std::size_t RuntimeScenePayload::GetMaterialCount() const noexcept
{
	return materials.size();
}

void RuntimeScenePayload::Reserve(std::size_t meshCount)
{
	meshes.reserve(meshCount);
	transforms.reserve(meshCount);
	materialHandles.reserve(meshCount);
}