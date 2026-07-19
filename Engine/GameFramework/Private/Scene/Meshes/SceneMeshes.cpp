#include "PCH.h"
#include "Scene/Meshes/SceneMeshes.h"

#include "World/GameWorld.h"
#include "World/GameWorldState.h"

SceneMeshes::SceneMeshes(GameWorld& world) noexcept : m_world(&world) {}

std::size_t SceneMeshes::GetMeshCount() const noexcept { return m_world->m_state->GetMeshCount(); }

EntityId SceneMeshes::GetMeshEntity(std::size_t index) const noexcept { return m_world->m_state->GetMeshEntity(index); }

SceneMeshView SceneMeshes::GetMesh(std::size_t index) const noexcept { return GetMesh(GetMeshEntity(index)); }

SceneMeshView SceneMeshes::GetMesh(EntityId entity) const noexcept { return SceneMeshView(*m_world, entity); }

MeshSnapshot SceneMeshes::CaptureSnapshot() const { return m_world->m_state->CaptureMeshes(); }

bool SceneMeshView::IsValid() const noexcept { return m_world != nullptr && m_world->m_state->IsAlive(m_entity); }

const Mesh* SceneMeshView::GetMesh() const noexcept
{
	return IsValid() ? m_world->m_state->ResolveMesh(m_entity) : nullptr;
}

bool SceneMeshView::IsVisible() const noexcept { return IsValid() && m_world->m_state->ReadVisibility(m_entity); }

void SceneMeshView::SetVisible(bool visible) noexcept
{
	if (IsValid())
	{
		m_world->m_state->WriteVisibility(m_entity, visible);
	}
}

bool SceneMeshView::IsSkeletal() const noexcept { return IsValid() && m_world->m_state->IsSkeletalMesh(m_entity); }

Transform SceneMeshView::GetTransform() const noexcept
{
	return IsValid() ? m_world->m_state->ReadTransform(m_entity) : Transform{};
}

void SceneMeshView::SetTransform(const Transform& transform) noexcept
{
	if (IsValid())
	{
		m_world->m_state->WriteTransform(m_entity, transform);
	}
}

MaterialHandle SceneMeshView::GetMaterialHandle() const noexcept
{
	return IsValid() ? m_world->m_state->ReadMeshMaterial(m_entity) : MaterialHandle::Invalid();
}

Assets::CookedAssetId SceneMeshView::GetMeshAssetId() const noexcept
{
	return IsValid() ? m_world->m_state->ReadMeshAssetId(m_entity) : Assets::InvalidCookedAssetId;
}

Assets::CookedAssetId SceneMeshView::GetSkeletonAssetId() const noexcept
{
	return IsValid() ? m_world->m_state->ReadSkeletonAssetId(m_entity) : Assets::InvalidCookedAssetId;
}

std::uint32_t SceneMeshView::GetSourceNodeIndex() const noexcept
{
	return IsValid() ? m_world->m_state->ReadMeshSourceNodeIndex(m_entity) : Assets::kInvalidCookedSceneSourceNodeIndex;
}
