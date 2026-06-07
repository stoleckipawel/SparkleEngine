#include "PCH.h"

#include "Scene/Meshes/SceneMeshes.h"

#include "Scene/Meshes/MeshComponent.h"

SceneMeshes::SceneMeshes() noexcept = default;

SceneMeshes::~SceneMeshes() noexcept = default;

void SceneMeshes::AppendMeshComponents(std::vector<std::unique_ptr<MeshComponent>>&& meshes)
{
	if (meshes.empty())
	{
		return;
	}

	m_meshes.reserve(m_meshes.size() + meshes.size());
	for (std::unique_ptr<MeshComponent>& mesh : meshes)
	{
		if (!mesh)
		{
			continue;
		}

		m_meshes.push_back(std::move(mesh));
	}
}

void SceneMeshes::AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& meshInstanceGroups)
{
	if (meshInstanceGroups.empty())
	{
		return;
	}

	m_meshInstanceGroups.reserve(m_meshInstanceGroups.size() + meshInstanceGroups.size());
	for (MeshInstanceGroupSnapshot& meshInstanceGroup : meshInstanceGroups)
	{
		m_meshInstanceGroups.push_back(meshInstanceGroup);
	}
}

bool SceneMeshes::SetMeshMaterial(SceneMeshInstanceIndex meshInstanceIndex, MaterialHandle materialHandle) noexcept
{
	if (meshInstanceIndex >= m_meshes.size() || !m_meshes[meshInstanceIndex])
	{
		return false;
	}

	m_meshes[meshInstanceIndex]->SetMaterialHandle(materialHandle);
	return true;
}

void SceneMeshes::Reset() noexcept
{
	m_meshes.clear();
	m_meshInstanceGroups.clear();
}

MeshSnapshot SceneMeshes::CaptureSnapshot() const
{
	MeshSnapshot snapshot;
	snapshot.meshInstances.reserve(m_meshes.size());
	snapshot.meshInstanceGroups = m_meshInstanceGroups;
	for (MeshInstanceGroupSnapshot& meshInstanceGroup : snapshot.meshInstanceGroups)
	{
		meshInstanceGroup.firstInstance = kInvalidSceneMeshInstanceIndex;
		meshInstanceGroup.instanceCount = 0;
	}

	for (const std::unique_ptr<MeshComponent>& mesh : m_meshes)
	{
		if (!mesh || !mesh->IsVisible() || !mesh->HasMesh())
		{
			continue;
		}

		MeshInstanceSnapshot meshInstance = {};
		meshInstance.mesh = mesh->GetMesh();
		DirectX::XMStoreFloat4x4(&meshInstance.worldMatrix, mesh->GetWorldMatrix());
		DirectX::XMStoreFloat3x4(&meshInstance.worldInvTranspose, mesh->GetWorldInverseTransposeMatrix());
		meshInstance.materialHandle = mesh->GetMaterialHandle();
		meshInstance.meshAssetId = mesh->GetMeshAssetId();
		meshInstance.skeletonAssetId = mesh->GetSkeletonAssetId();
		meshInstance.meshKind = mesh->GetMeshKind();
		meshInstance.meshAssetIndex = mesh->GetMeshAssetIndex();
		meshInstance.instanceGroupIndex = mesh->GetMeshInstanceGroupIndex();
		if (meshInstance.instanceGroupIndex < snapshot.meshInstanceGroups.size())
		{
			MeshInstanceGroupSnapshot& meshInstanceGroup = snapshot.meshInstanceGroups[meshInstance.instanceGroupIndex];
			if (meshInstanceGroup.instanceCount == 0)
			{
				meshInstanceGroup.firstInstance = static_cast<SceneMeshInstanceIndex>(snapshot.meshInstances.size());
			}
			++meshInstanceGroup.instanceCount;
		}
		snapshot.meshInstances.push_back(meshInstance);
	}

	return snapshot;
}
