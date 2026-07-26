#include "PCH.h"
#include "SceneData/RenderWorld.h"

#include <set>

RenderWorldApplyStatus RenderWorld::Apply(const RenderWorldDelta& delta, std::string& diagnostic)
{
	const RenderWorldApplyStatus validationStatus = Validate(delta, diagnostic);
	if (validationStatus != RenderWorldApplyStatus::Applied) return validationStatus;

	if (delta.ResetScene)
	{
		m_proxies.clear();
		m_availableGpuSceneSlots.clear();
		m_retiredGpuSceneSlots.clear();
		m_nextGpuSceneSlot = 0;
		m_historyReset = true;
	}
	if (delta.ResetScene || !delta.Creates.empty() || !delta.Updates.empty() ||
	    !delta.Destroys.empty() || delta.InstanceGroups.Published ||
	    delta.Sky.Published)
	{
		++m_structuralRevision;
	}
	if (delta.Materials)
	{
		++m_materialRevision;
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
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		diagnostic = "Render-world identity is invalid.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (delta.SceneGeneration < m_sceneGeneration)
	{
		diagnostic = "Render-world delta is stale.";
		return RenderWorldApplyStatus::Stale;
	}
	if (delta.SceneGeneration == m_sceneGeneration &&
	    delta.SequenceNumber == m_sequenceNumber)
	{
		diagnostic = "Render-world delta is duplicated.";
		return RenderWorldApplyStatus::Duplicate;
	}
	if (delta.SceneGeneration == m_sceneGeneration &&
	    m_sequenceNumber != 0 &&
	    delta.SequenceNumber != m_sequenceNumber + 1)
	{
		diagnostic = "Render-world delta is out of order.";
		return RenderWorldApplyStatus::OutOfOrder;
	}
	if (delta.SceneGeneration > m_sceneGeneration && !delta.ResetScene)
	{
		diagnostic = "A new render-world generation requires a reset delta.";
		return RenderWorldApplyStatus::Rejected;
	}

	std::set<RenderObjectId> touched;
	for (RenderObjectId object : delta.Destroys)
	{
		if (!object.IsValid() || delta.ResetScene || Find(object) == nullptr ||
		    !touched.insert(object).second)
		{
			diagnostic = "Render-world destroy is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Static.Mesh.IsValid() ||
		    (!delta.ResetScene && Find(create.Object) != nullptr) ||
		    !touched.insert(create.Object).second)
		{
			diagnostic = "Render-world create is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (!update.Object.IsValid() || !update.Static.Mesh.IsValid() ||
		    delta.ResetScene || Find(update.Object) == nullptr ||
		    !touched.insert(update.Object).second)
		{
			diagnostic = "Render-world update is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	diagnostic.clear();
	return RenderWorldApplyStatus::Applied;
}

void RenderWorld::ApplyDestroys(const RenderWorldDelta& delta)
{
	for (RenderObjectId object : delta.Destroys)
	{
		const auto proxy = m_proxies.find(object);
		if (proxy == m_proxies.end())
		{
			continue;
		}
		RetireGpuSceneSlot(
		    proxy->second.GpuSceneSlot,
		    delta.SequenceNumber);
		m_proxies.erase(proxy);
	}
}

void RenderWorld::ApplyCreates(const RenderWorldDelta& delta)
{
	for (const RenderObjectCreate& create : delta.Creates)
		m_proxies.emplace(
		    create.Object,
		    RenderProxy{
		        create.Object,
		        create.Static,
		        AllocateGpuSceneSlot(delta.SequenceNumber)});
}

std::uint32_t RenderWorld::AllocateGpuSceneSlot(
    std::uint64_t sequenceNumber)
{
	constexpr std::uint64_t retirementDistance = 3;
	for (std::size_t index = 0; index < m_retiredGpuSceneSlots.size();)
	{
		const RetiredGpuSceneSlot& retired =
		    m_retiredGpuSceneSlots[index];
		if (sequenceNumber <=
		    retired.SequenceNumber + retirementDistance)
		{
			++index;
			continue;
		}
		m_availableGpuSceneSlots.push_back(retired.Slot);
		m_retiredGpuSceneSlots.erase(
		    m_retiredGpuSceneSlots.begin() + index);
	}
	if (!m_availableGpuSceneSlots.empty())
	{
		const std::uint32_t slot =
		    m_availableGpuSceneSlots.back();
		m_availableGpuSceneSlots.pop_back();
		return slot;
	}
	return m_nextGpuSceneSlot++;
}

void RenderWorld::RetireGpuSceneSlot(
    std::uint32_t slot,
    std::uint64_t sequenceNumber)
{
	m_retiredGpuSceneSlots.push_back(
	    RetiredGpuSceneSlot{
	        .Slot = slot,
	        .SequenceNumber = sequenceNumber});
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
