#include "PCH.h"
#include "SceneData/RenderWorld.h"

#include <set>

RenderWorldApplyStatus RenderWorld::Apply(const RenderWorldDelta& delta, std::string& diagnostic)
{
	const RenderWorldApplyStatus sequenceStatus = ValidateSequence(delta, diagnostic);
	if (sequenceStatus != RenderWorldApplyStatus::Applied) return sequenceStatus;
	if (!ValidateOperations(delta, diagnostic)) return RenderWorldApplyStatus::Rejected;
	if (delta.ResetScene)
	{
		m_proxies.clear();
		m_historyReset = true;
	}
	ApplyDestroys(delta);
	ApplyCreates(delta);
	ApplyUpdates(delta);
	PublishResources(delta);
	m_sceneGeneration = delta.SceneGeneration;
	m_sequenceNumber = delta.SequenceNumber;
	diagnostic.clear();
	return RenderWorldApplyStatus::Applied;
}

RenderWorldApplyStatus RenderWorld::ValidateSequence(const RenderWorldDelta& delta, std::string& diagnostic) const
{
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		diagnostic = "Render-world scene generation and sequence must be non-zero.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (delta.SceneGeneration < m_sceneGeneration)
	{
		diagnostic = "Render-world delta belongs to a stale scene generation.";
		return RenderWorldApplyStatus::Stale;
	}
	if (delta.SceneGeneration == m_sceneGeneration && delta.SequenceNumber == m_sequenceNumber)
	{
		diagnostic = "Render-world delta is a duplicate.";
		return RenderWorldApplyStatus::Duplicate;
	}
	if (delta.SceneGeneration == m_sceneGeneration && m_sequenceNumber != 0 && delta.SequenceNumber != m_sequenceNumber + 1)
	{
		diagnostic = "Render-world delta is out of order.";
		return RenderWorldApplyStatus::OutOfOrder;
	}
	if (delta.SceneGeneration > m_sceneGeneration && !delta.ResetScene)
	{
		diagnostic = "A new render scene generation must begin with ResetScene.";
		return RenderWorldApplyStatus::Rejected;
	}
	return RenderWorldApplyStatus::Applied;
}

bool RenderWorld::ValidateOperations(const RenderWorldDelta& delta, std::string& diagnostic) const
{
	const bool reset = delta.ResetScene;
	if (reset && (!delta.Materials || !delta.Textures))
	{
		diagnostic = "Render-world reset must publish immutable material and texture tables.";
		return false;
	}
	for (RenderObjectId object : delta.Destroys)
	{
		if (reset || !m_proxies.contains(object))
		{
			diagnostic = "Render-world destroy referenced an unavailable object.";
			return false;
		}
	}
	std::set<RenderObjectId> creates;
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Mesh.IsValid() || (!reset && m_proxies.contains(create.Object)) ||
		    !creates.insert(create.Object).second)
		{
			diagnostic = "Render-world create contained an invalid or duplicate identity/asset handle.";
			return false;
		}
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if ((reset || !m_proxies.contains(update.Object)) && !creates.contains(update.Object))
		{
			diagnostic = "Render-world update referenced an unavailable object.";
			return false;
		}
	}
	return true;
}

void RenderWorld::ApplyDestroys(const RenderWorldDelta& delta)
{
	for (RenderObjectId object : delta.Destroys) m_proxies.erase(object);
}

void RenderWorld::ApplyCreates(const RenderWorldDelta& delta)
{
	for (const RenderObjectCreate& create : delta.Creates)
		m_proxies.emplace(create.Object, RenderProxy{create.Object, create.Mesh, create.Material,
		                                              create.SkeletonAssetId, create.MeshKind,
		                                              create.MeshAssetIndex, create.InstanceGroupIndex});
}

void RenderWorld::ApplyUpdates(const RenderWorldDelta& delta)
{
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		RenderProxy& proxy = m_proxies.at(update.Object);
		proxy.Material = update.Material;
		proxy.InstanceGroupIndex = update.InstanceGroupIndex;
	}
}

void RenderWorld::PublishResources(const RenderWorldDelta& delta)
{
	if (delta.Materials) m_materials = *delta.Materials;
	if (delta.Textures) m_textures = *delta.Textures;
	m_sky = delta.Sky;
	m_instanceGroups = delta.InstanceGroups;
}

const RenderProxy* RenderWorld::Find(RenderObjectId object) const noexcept
{
	const auto iterator = m_proxies.find(object);
	return iterator == m_proxies.end() ? nullptr : &iterator->second;
}
