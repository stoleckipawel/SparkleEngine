#include "PCH.h"

#include "RenderSceneSnapshotCache.h"

#include "Scene/Mesh.h"
#include "Scene/Scene.h"

const RenderSceneSnapshot& RenderSceneSnapshotCache::Capture(const Scene& scene)
{
	m_snapshot.materials = scene.GetLoadedMaterials();
	m_snapshot.meshes.clear();

	if (!scene.HasMeshes())
	{
		return m_snapshot;
	}

	const auto& meshes = scene.GetMeshes();
	m_snapshot.meshes.reserve(meshes.size());

	for (const auto& mesh : meshes)
	{
		RenderSceneMeshRecord record{};
		record.mesh = mesh.get();
		DirectX::XMStoreFloat4x4(&record.worldMatrix, mesh->GetWorldMatrix());
		DirectX::XMStoreFloat3x4(&record.worldInvTranspose, mesh->GetWorldInverseTransposeMatrix());
		record.materialId = mesh->GetMaterialId();
		m_snapshot.meshes.push_back(record);
	}

	return m_snapshot;
}

void RenderSceneSnapshotCache::Reset() noexcept
{
	m_snapshot.meshes.clear();
	m_snapshot.materials.clear();
}