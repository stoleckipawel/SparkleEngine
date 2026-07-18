#include "PCH.h"
#include "World/SceneWorld.h"

#include "World/ECS/Components/RenderingComponents.h"

namespace ECS
{
	void SceneWorld::Clear() noexcept
	{
		m_registry.Clear();
		m_meshResources.Clear();
		m_animationResources.Clear();
		m_deformationStates.Clear();
		m_meshInstanceGroups.clear();
		m_activeCamera = EntityId::Invalid();
		m_nextCameraIdentity = 0;
		m_nextLightIdentity = 0;
	}

	bool SceneWorld::Destroy(EntityId entity) noexcept
	{
		const MeshInstance* mesh = m_registry.Get<MeshInstance>(entity);
		const SceneMeshResourceHandle meshResource = mesh == nullptr ? SceneMeshResourceHandle{} : mesh->Resource;
		const MorphState* morph = m_registry.Get<MorphState>(entity);
		const SceneStateHandle morphState = morph == nullptr ? SceneStateHandle{} : morph->Weights;
		const bool destroyed = m_registry.Destroy(entity);
		if (!destroyed)
		{
			return false;
		}
		if (entity == m_activeCamera)
		{
			m_activeCamera = EntityId::Invalid();
		}
		if (meshResource.IsValid())
		{
			m_meshResources.Remove(meshResource);
		}
		if (morphState.IsValid())
		{
			m_deformationStates.Remove(morphState);
		}
		return true;
	}
}
