#include "PCH.h"
#include "SceneData/RenderWorld.h"

#include "Meshes/GpuMeshCache.h"
#include "SceneData/GpuScene/GpuSceneSlotAllocator.h"

#include <algorithm>
#include <utility>

RenderWorld::RenderWorld(RhiCommandSubmissionService* submissionService, GpuMeshCache& gpuMeshCache) :
    m_gpuSceneSlots(std::make_unique<GpuSceneSlotAllocator>(submissionService)),
    m_gpuMeshCache(&gpuMeshCache)
{
}

RenderWorld::~RenderWorld() noexcept = default;

RenderWorldApplyStatus RenderWorld::ApplyFrame(
    const RenderSceneDelta& delta,
    const RenderSceneDynamicData& dynamic,
    std::string& diagnostic)
{
	const RenderWorldApplyStatus validationStatus = ValidateDelta(delta, diagnostic);
	if (validationStatus != RenderWorldApplyStatus::Applied)
	{
		return validationStatus;
	}
	if (!ValidateDynamic(dynamic, delta, diagnostic))
	{
		return RenderWorldApplyStatus::Rejected;
	}

	const RenderWorldApplyStatus applyStatus = ApplyValidatedDelta(delta, diagnostic);
	if (applyStatus == RenderWorldApplyStatus::Applied)
	{
		ApplyDynamic(dynamic);
	}
	return applyStatus;
}

void RenderWorld::PromoteResidentGpuMeshes() noexcept
{
	bool changed = false;
	for (RenderProxy& proxy : m_proxies)
	{
		if (proxy.HasPendingStatic)
		{
			if (!proxy.PendingGpuMesh)
			{
				proxy.PendingGpuMesh = m_gpuMeshCache->Request(proxy.PendingStatic.Mesh);
			}

			if (m_gpuMeshCache->Resolve(proxy.PendingGpuMesh) != nullptr)
			{
				proxy.Static = std::move(proxy.PendingStatic);
				proxy.GpuMesh = proxy.PendingGpuMesh;
				proxy.PendingGpuMesh = {};
				proxy.GpuMeshResident = true;
				proxy.HasPendingStatic = false;
				changed = true;
			}
			continue;
		}

		if (!proxy.GpuMesh)
		{
			proxy.GpuMesh = m_gpuMeshCache->Request(proxy.Static.Mesh);
		}

		if (!proxy.GpuMeshResident && m_gpuMeshCache->Resolve(proxy.GpuMesh) != nullptr)
		{
			proxy.GpuMeshResident = true;
			changed = true;
		}
	}

	if (changed)
	{
		++m_structuralRevision;
	}

	RetainReferencedGpuMeshes();
}

RenderWorldApplyStatus RenderWorld::ApplyValidatedDelta(const RenderSceneDelta& delta, std::string& diagnostic)
{
	std::vector<GpuMeshHandle> createMeshes;
	std::vector<GpuMeshHandle> updateMeshes;
	ResolveGpuMeshes(delta, createMeshes, updateMeshes);

	if (delta.ResetScene)
	{
		for (const RenderProxy& proxy : m_proxies)
		{
			m_gpuSceneSlots->Retire(proxy.GpuSceneSlot);
		}
		m_proxies.clear();
		m_historyReset = true;
	}
	if (delta.ResetScene || !delta.Creates.empty() || !delta.Updates.empty() || !delta.Destroys.empty() || delta.InstanceGroups.Published
	    || delta.Sky.Published)
	{
		++m_structuralRevision;
	}
	if (delta.Materials)
	{
		++m_materialRevision;
	}
	ApplyDestroys(delta);
	ApplyCreates(delta, createMeshes);
	ApplyUpdates(delta, updateMeshes);
	PublishResources(delta);
	RetainReferencedGpuMeshes();

	m_sceneGeneration = delta.SceneGeneration;
	m_sequenceNumber = delta.SequenceNumber;
	diagnostic.clear();
	return RenderWorldApplyStatus::Applied;
}

bool RenderWorld::ValidateDynamic(const RenderSceneDynamicData& dynamic, const RenderSceneDelta& delta, std::string& diagnostic) const
{
	if (!HasStrictlyOrderedDynamicObjects(dynamic.Objects) || !HasStrictlyOrderedJointMatrixRanges(dynamic.JointMatrixRanges)
	    || !HasStrictlyOrderedMorphWeightRanges(dynamic.MorphWeightRanges))
	{
		diagnostic = "Render-frame object updates are unordered or duplicated.";
		return false;
	}

	for (const RenderObjectDynamicData& object : dynamic.Objects)
	{
		if (!object.Object.IsValid() || !IsObjectAvailable(object.Object, delta))
		{
			diagnostic = "Render-frame object update is invalid.";
			return false;
		}
	}

	for (const RenderObjectCreate& create : delta.Creates)
	{
		const auto dynamicObject = std::lower_bound(
		    dynamic.Objects.begin(),
		    dynamic.Objects.end(),
		    create.Object,
		    [](const RenderObjectDynamicData& object, RenderObjectId identity) { return object.Object < identity; });
		const bool hasDynamicData = dynamicObject != dynamic.Objects.end() && dynamicObject->Object == create.Object;
		if (!hasDynamicData)
		{
			diagnostic = "Render scene create has no dynamic object data.";
			return false;
		}
	}

	for (const RenderJointMatrixRange& range : dynamic.JointMatrixRanges)
	{
		if (!range.Object.IsValid() || !IsObjectAvailable(range.Object, delta) || range.JointMatrixOffset > dynamic.JointMatrices.size()
		    || range.JointMatrixCount > dynamic.JointMatrices.size() - range.JointMatrixOffset)
		{
			diagnostic = "Render-frame joint-matrix range is invalid.";
			return false;
		}
	}

	for (const RenderMorphWeightRange& morphWeightRange : dynamic.MorphWeightRanges)
	{
		if (!morphWeightRange.Object.IsValid() || !IsObjectAvailable(morphWeightRange.Object, delta)
		    || morphWeightRange.WeightOffset > dynamic.MorphWeights.size()
		    || morphWeightRange.WeightCount > dynamic.MorphWeights.size() - morphWeightRange.WeightOffset)
		{
			diagnostic = "Render-frame morph-weight range is invalid.";
			return false;
		}
	}

	diagnostic.clear();
	return true;
}

void RenderWorld::ApplyDynamic(const RenderSceneDynamicData& dynamic) noexcept
{
	for (const RenderObjectDynamicData& object : dynamic.Objects)
	{
		RenderProxy* proxy = FindMutable(object.Object);
		if (proxy != nullptr)
		{
			proxy->Dynamic = object;
		}
	}
}

RenderWorldApplyStatus RenderWorld::ValidateDelta(const RenderSceneDelta& delta, std::string& diagnostic) const
{
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		diagnostic = "Render scene delta identity is invalid.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (delta.SceneGeneration < m_sceneGeneration)
	{
		diagnostic = "Render scene delta is stale.";
		return RenderWorldApplyStatus::Stale;
	}
	if (delta.SceneGeneration == m_sceneGeneration && delta.SequenceNumber == m_sequenceNumber)
	{
		diagnostic = "Render scene delta is duplicated.";
		return RenderWorldApplyStatus::Duplicate;
	}
	if (delta.SceneGeneration == m_sceneGeneration && m_sequenceNumber != 0 && delta.SequenceNumber != m_sequenceNumber + 1)
	{
		diagnostic = "Render scene delta is out of order.";
		return RenderWorldApplyStatus::OutOfOrder;
	}
	if (delta.SceneGeneration > m_sceneGeneration && !delta.ResetScene)
	{
		diagnostic = "A new render scene generation requires a reset delta.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (!HasOrderedDeltaObjects(delta))
	{
		diagnostic = "Render scene object changes are not deterministically ordered.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (HasConflictingDeltaObjects(delta))
	{
		diagnostic = "Render scene object changes are duplicated or conflicting.";
		return RenderWorldApplyStatus::Rejected;
	}

	for (RenderObjectId object : delta.Destroys)
	{
		if (!object.IsValid() || delta.ResetScene || Find(object) == nullptr)
		{
			diagnostic = "Render scene destroy is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Static.Mesh.IsValid() || (!delta.ResetScene && Find(create.Object) != nullptr))
		{
			diagnostic = "Render scene create is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (!update.Object.IsValid() || !update.Static.Mesh.IsValid() || delta.ResetScene || Find(update.Object) == nullptr)
		{
			diagnostic = "Render scene update is invalid.";
			return RenderWorldApplyStatus::Rejected;
		}
	}
	diagnostic.clear();
	return RenderWorldApplyStatus::Applied;
}

void RenderWorld::ApplyDestroys(const RenderSceneDelta& delta)
{
	for (RenderObjectId object : delta.Destroys)
	{
		const auto proxy = std::lower_bound(
		    m_proxies.begin(),
		    m_proxies.end(),
		    object,
		    [](const RenderProxy& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		if (proxy == m_proxies.end() || proxy->Object != object)
		{
			continue;
		}

		m_gpuSceneSlots->Retire(proxy->GpuSceneSlot);
		m_proxies.erase(proxy);
	}
}

void RenderWorld::ResolveGpuMeshes(
    const RenderSceneDelta& delta,
    std::vector<GpuMeshHandle>& createMeshes,
    std::vector<GpuMeshHandle>& updateMeshes)
{
	createMeshes.reserve(delta.Creates.size());
	updateMeshes.reserve(delta.Updates.size());

	for (const RenderObjectCreate& create : delta.Creates)
	{
		createMeshes.push_back(m_gpuMeshCache->Request(create.Static.Mesh));
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		updateMeshes.push_back(m_gpuMeshCache->Request(update.Static.Mesh));
	}
}

void RenderWorld::ApplyCreates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes)
{
	for (std::size_t index = 0; index < delta.Creates.size(); ++index)
	{
		const RenderObjectCreate& create = delta.Creates[index];
		const auto insertion = std::lower_bound(
		    m_proxies.begin(),
		    m_proxies.end(),
		    create.Object,
		    [](const RenderProxy& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		m_proxies.insert(
		    insertion,
		    RenderProxy{
		        .Object = create.Object,
		        .Static = create.Static,
		        .GpuMesh = meshes[index],
		        .GpuSceneSlot = m_gpuSceneSlots->Allocate(),
		        .GpuMeshResident = m_gpuMeshCache->Resolve(meshes[index]) != nullptr});
	}
}

void RenderWorld::ApplyUpdates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes)
{
	for (std::size_t index = 0; index < delta.Updates.size(); ++index)
	{
		const RenderObjectUpdate& update = delta.Updates[index];
		RenderProxy* proxy = FindMutable(update.Object);
		if (proxy == nullptr)
		{
			continue;
		}

		if (m_gpuMeshCache->Resolve(meshes[index]) != nullptr)
		{
			proxy->Static = update.Static;
			proxy->GpuMesh = meshes[index];
			proxy->PendingStatic = {};
			proxy->PendingGpuMesh = {};
			proxy->GpuMeshResident = true;
			proxy->HasPendingStatic = false;
		}
		else
		{
			proxy->PendingStatic = update.Static;
			proxy->PendingGpuMesh = meshes[index];
			proxy->HasPendingStatic = true;
		}
	}
}

void RenderWorld::PublishResources(const RenderSceneDelta& delta)
{
	if (delta.Materials)
		m_materials = *delta.Materials;
	if (delta.Textures)
		m_textures = *delta.Textures;
	if (delta.Sky.Published)
		m_sky = delta.Sky.Value;
	if (delta.InstanceGroups.Published)
		m_instanceGroups = delta.InstanceGroups.Values;
}

void RenderWorld::RetainReferencedGpuMeshes() noexcept
{
	std::vector<GpuMeshHandle> handles;
	handles.reserve(m_proxies.size() * 2u);
	for (const RenderProxy& proxy : m_proxies)
	{
		if (proxy.GpuMesh)
		{
			handles.push_back(proxy.GpuMesh);
		}
		if (proxy.PendingGpuMesh)
		{
			handles.push_back(proxy.PendingGpuMesh);
		}
	}

	m_gpuMeshCache->RetainOnly(handles);
}

bool RenderWorld::IsObjectAvailable(RenderObjectId object, const RenderSceneDelta& delta) const noexcept
{
	const auto created = std::lower_bound(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    object,
	    [](const RenderObjectCreate& create, RenderObjectId identity) { return create.Object < identity; });
	if (created != delta.Creates.end() && created->Object == object)
	{
		return true;
	}

	if (delta.ResetScene)
	{
		return false;
	}

	const bool destroyed = std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), object);
	return !destroyed && Find(object) != nullptr;
}

bool RenderWorld::HasOrderedDeltaObjects(const RenderSceneDelta& delta) noexcept
{
	const bool createsOrdered = std::is_sorted(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    [](const RenderObjectCreate& left, const RenderObjectCreate& right) { return left.Object < right.Object; });
	const bool updatesOrdered = std::is_sorted(
	    delta.Updates.begin(),
	    delta.Updates.end(),
	    [](const RenderObjectUpdate& left, const RenderObjectUpdate& right) { return left.Object < right.Object; });
	return createsOrdered && updatesOrdered && std::is_sorted(delta.Destroys.begin(), delta.Destroys.end());
}

bool RenderWorld::HasConflictingDeltaObjects(const RenderSceneDelta& delta) noexcept
{
	const auto duplicateCreates = std::adjacent_find(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    [](const RenderObjectCreate& left, const RenderObjectCreate& right) { return left.Object == right.Object; });
	const auto duplicateUpdates = std::adjacent_find(
	    delta.Updates.begin(),
	    delta.Updates.end(),
	    [](const RenderObjectUpdate& left, const RenderObjectUpdate& right) { return left.Object == right.Object; });
	const auto duplicateDestroys = std::adjacent_find(delta.Destroys.begin(), delta.Destroys.end());
	if (duplicateCreates != delta.Creates.end() || duplicateUpdates != delta.Updates.end() || duplicateDestroys != delta.Destroys.end())
	{
		return true;
	}

	for (const RenderObjectCreate& create : delta.Creates)
	{
		const auto update = std::lower_bound(
		    delta.Updates.begin(),
		    delta.Updates.end(),
		    create.Object,
		    [](const RenderObjectUpdate& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		if ((update != delta.Updates.end() && update->Object == create.Object)
		    || std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), create.Object))
		{
			return true;
		}
	}

	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), update.Object))
		{
			return true;
		}
	}
	return false;
}

bool RenderWorld::HasStrictlyOrderedDynamicObjects(std::span<const RenderObjectDynamicData> objects) noexcept
{
	for (std::size_t index = 1u; index < objects.size(); ++index)
	{
		if (!(objects[index - 1u].Object < objects[index].Object))
		{
			return false;
		}
	}
	return true;
}

bool RenderWorld::HasStrictlyOrderedJointMatrixRanges(std::span<const RenderJointMatrixRange> ranges) noexcept
{
	for (std::size_t index = 1u; index < ranges.size(); ++index)
	{
		if (!(ranges[index - 1u].Object < ranges[index].Object))
		{
			return false;
		}
	}
	return true;
}

bool RenderWorld::HasStrictlyOrderedMorphWeightRanges(std::span<const RenderMorphWeightRange> ranges) noexcept
{
	for (std::size_t index = 1u; index < ranges.size(); ++index)
	{
		if (!(ranges[index - 1u].Object < ranges[index].Object))
		{
			return false;
		}
	}
	return true;
}

const RenderProxy* RenderWorld::Find(RenderObjectId object) const noexcept
{
	const auto proxy = std::lower_bound(
	    m_proxies.begin(),
	    m_proxies.end(),
	    object,
	    [](const RenderProxy& candidate, RenderObjectId identity) { return candidate.Object < identity; });
	return proxy == m_proxies.end() || proxy->Object != object ? nullptr : &*proxy;
}

RenderProxy* RenderWorld::FindMutable(RenderObjectId object) noexcept
{
	return const_cast<RenderProxy*>(std::as_const(*this).Find(object));
}

bool RenderWorld::ConsumeHistoryReset() noexcept
{
	return std::exchange(m_historyReset, false);
}
