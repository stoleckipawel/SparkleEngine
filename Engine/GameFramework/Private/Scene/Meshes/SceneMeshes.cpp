#include "PCH.h"
#include "Scene/Meshes/SceneMeshes.h"

#include "Scene/GameScene.h"
#include "World/SceneMeshInstanceData.h"
#include "World/SceneWorld.h"

SceneMeshes::SceneMeshes(GameScene& scene) noexcept : m_scene(&scene) {}

std::size_t SceneMeshes::GetMeshCount() const noexcept { return m_scene->m_world->GetMeshCount(); }

std::size_t SceneMeshes::GetMeshInstanceGroupCount() const noexcept { return m_scene->m_world->GetMeshInstanceGroupCount(); }

EntityId SceneMeshes::GetMeshEntity(std::size_t index) const noexcept { return m_scene->m_world->GetMeshEntity(index); }

SceneMeshView SceneMeshes::GetMesh(std::size_t index) const noexcept { return GetMesh(GetMeshEntity(index)); }

SceneMeshView SceneMeshes::GetMesh(EntityId entity) const noexcept { return SceneMeshView(*m_scene, entity); }

bool SceneMeshes::AppendMesh(ECS::SceneMeshInstanceData&& instance)
{
	return m_scene->m_world->AddMesh(std::move(instance)).IsValid();
}

void SceneMeshes::AppendMeshInstanceGroups(std::vector<MeshInstanceGroupSnapshot>&& groups)
{
	m_scene->m_world->AppendMeshInstanceGroups(std::move(groups));
}

bool SceneMeshes::SetMeshMaterial(SceneMeshInstanceIndex meshInstanceIndex, MaterialHandle materialHandle) noexcept
{
	return m_scene->m_world->WriteMeshMaterial(GetMeshEntity(meshInstanceIndex), materialHandle);
}

MeshSnapshot SceneMeshes::CaptureSnapshot() const { return m_scene->m_world->CaptureMeshes(); }

bool SceneMeshView::IsValid() const noexcept { return m_scene != nullptr && m_scene->m_world->IsAlive(m_entity); }

const Mesh* SceneMeshView::GetMesh() const noexcept
{
	return IsValid() ? m_scene->m_world->ResolveMesh(m_entity) : nullptr;
}

bool SceneMeshView::IsVisible() const noexcept { return IsValid() && m_scene->m_world->ReadVisibility(m_entity); }

void SceneMeshView::SetVisible(bool visible) noexcept
{
	if (IsValid())
	{
		m_scene->m_world->WriteVisibility(m_entity, visible);
	}
}

bool SceneMeshView::IsSkeletal() const noexcept { return IsValid() && m_scene->m_world->IsSkeletalMesh(m_entity); }

Transform SceneMeshView::GetTransform() const noexcept
{
	return IsValid() ? m_scene->m_world->ReadTransform(m_entity) : Transform{};
}

void SceneMeshView::SetTransform(const Transform& transform) noexcept
{
	if (IsValid())
	{
		m_scene->m_world->WriteTransform(m_entity, transform);
	}
}

MaterialHandle SceneMeshView::GetMaterialHandle() const noexcept
{
	return IsValid() ? m_scene->m_world->ReadMeshMaterial(m_entity) : MaterialHandle::Invalid();
}

void SceneMeshView::SetMaterialHandle(MaterialHandle material) noexcept
{
	if (IsValid())
	{
		m_scene->m_world->WriteMeshMaterial(m_entity, material);
	}
}

Assets::CookedAssetId SceneMeshView::GetMeshAssetId() const noexcept
{
	return IsValid() ? m_scene->m_world->ReadMeshAssetId(m_entity) : Assets::InvalidCookedAssetId;
}

Assets::CookedAssetId SceneMeshView::GetSkeletonAssetId() const noexcept
{
	return IsValid() ? m_scene->m_world->ReadSkeletonAssetId(m_entity) : Assets::InvalidCookedAssetId;
}

std::uint32_t SceneMeshView::GetSourceNodeIndex() const noexcept
{
	return IsValid() ? m_scene->m_world->ReadMeshSourceNodeIndex(m_entity) : Assets::kInvalidCookedSceneSourceNodeIndex;
}
