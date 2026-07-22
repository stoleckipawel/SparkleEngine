#include "PCH.h"
#include "SceneData/RenderWorld.h"

#include "SceneData/Validation/RenderWorldDeltaValidator.h"

RenderWorldApplyStatus RenderWorld::Apply(const RenderWorldDelta& delta, std::string& diagnostic)
{
	const RenderWorldApplyStatus validationStatus = Validate(delta, diagnostic);
	if (validationStatus != RenderWorldApplyStatus::Applied) return validationStatus;

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

RenderWorldApplyStatus RenderWorld::Validate(const RenderWorldDelta& delta, std::string& diagnostic) const
{
	return RenderWorldDeltaValidator::Validate(*this, delta, diagnostic);
}

void RenderWorld::ApplyDestroys(const RenderWorldDelta& delta)
{
	for (RenderObjectId object : delta.Destroys) m_proxies.erase(object);
}

void RenderWorld::ApplyCreates(const RenderWorldDelta& delta)
{
	for (const RenderObjectCreate& create : delta.Creates)
		m_proxies.emplace(create.Object, RenderProxy{create.Object, create.Static});
}

void RenderWorld::ApplyUpdates(const RenderWorldDelta& delta)
{
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		const auto proxy = m_proxies.find(update.Object);
		if (proxy != m_proxies.end()) proxy->second.Static = update.Static;
	}
}

void RenderWorld::PublishResources(const RenderWorldDelta& delta)
{
	if (delta.Materials) m_materials = *delta.Materials;
	if (delta.Textures) m_textures = *delta.Textures;
	if (delta.Sky.Published) m_sky = delta.Sky.Value;
	if (delta.InstanceGroups.Published) m_instanceGroups = delta.InstanceGroups.Values;
}

const RenderProxy* RenderWorld::Find(RenderObjectId object) const noexcept
{
	const auto iterator = m_proxies.find(object);
	return iterator == m_proxies.end() ? nullptr : &iterator->second;
}
