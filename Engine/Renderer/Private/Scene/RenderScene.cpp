#include "PCH.h"
#include "Scene/RenderScene.h"

#include "Meshes/GpuMeshCache.h"
#include "Scene/Materials/MaterialCache.h"
#include "Scene/Preparation/RenderDeformationPreparation.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"
#include "SceneData/GpuScene/GpuSceneSlotAllocator.h"

#include <algorithm>
#include <utility>

RenderScene::RenderScene(
    RhiCommandSubmissionService* submissionService,
    GpuMeshCache& gpuMeshCache,
    TextureCache& textureCache,
    RenderHardwareInterface& renderHardwareInterface) :
    m_gpuSceneSlots(std::make_unique<GpuSceneSlotAllocator>(submissionService)),
    m_gpuMeshCache(&gpuMeshCache),
    m_materialCache(std::make_unique<MaterialCache>(textureCache, renderHardwareInterface))
{
}

RenderScene::~RenderScene() noexcept = default;

RenderSceneApplyStatus RenderScene::Apply(const RenderSceneDelta& delta, RenderSceneDynamicData dynamic, std::string& diagnostic)
{
	const RenderSceneApplyStatus validationStatus = ValidateDelta(delta, diagnostic);
	if (validationStatus != RenderSceneApplyStatus::Applied)
	{
		return validationStatus;
	}
	if (!ValidateDynamic(dynamic, delta, diagnostic))
	{
		return RenderSceneApplyStatus::Rejected;
	}

	const RenderSceneApplyStatus applyStatus = ApplyValidatedDelta(delta, diagnostic);
	if (applyStatus == RenderSceneApplyStatus::Applied)
	{
		ApplyDynamic(std::move(dynamic));
	}
	return applyStatus;
}

void RenderScene::BuildMaterials(PreparedRenderScene& preparedScene)
{
	m_materialCache->BuildMaterials(m_materials, m_materialRevision, preparedScene);
}

DirectX::XMFLOAT4X4 RenderScene::ResolvePreviousWorldMatrix(const RenderPrimitive& primitive) const noexcept
{
	if (primitive.GpuSceneSlot < m_previousWorldTransforms.size())
	{
		const PreviousWorldTransform& previous = m_previousWorldTransforms[primitive.GpuSceneSlot];
		if (previous.Object == primitive.Object)
		{
			return previous.WorldMatrix;
		}
	}
	return primitive.Dynamic.WorldMatrix;
}

std::span<const DirectX::XMFLOAT4X4> RenderScene::FindPreviousJointMatrices(RenderObjectId primitiveId) const noexcept
{
	const auto history = m_jointMatrixHistory.find(primitiveId);
	return history != m_jointMatrixHistory.end() ? std::span<const DirectX::XMFLOAT4X4>{history->second}
	                                             : std::span<const DirectX::XMFLOAT4X4>{};
}

std::span<const float> RenderScene::FindPreviousMorphWeights(RenderObjectId primitiveId) const noexcept
{
	const auto history = m_morphWeightHistory.find(primitiveId);
	return history != m_morphWeightHistory.end() ? std::span<const float>{history->second} : std::span<const float>{};
}

void RenderScene::CommitContinuity(std::span<const PreparedRenderPrimitive> primitives, const RenderDeformationWork& deformation)
{
	CommitPreviousWorldTransforms(primitives);
	CommitJointMatrixContinuity(deformation);
	CommitMorphWeightContinuity(deformation);
}

void RenderScene::CommitPreviousWorldTransforms(std::span<const PreparedRenderPrimitive> primitives)
{
	m_previousWorldTransforms.clear();

	std::uint32_t requiredSlotCount = 0u;
	for (const PreparedRenderPrimitive& primitive : primitives)
	{
		requiredSlotCount = (std::max) (requiredSlotCount, primitive.Draw.Source.GpuSceneSlot + 1u);
	}
	m_previousWorldTransforms.resize(requiredSlotCount);

	for (const PreparedRenderPrimitive& primitive : primitives)
	{
		if (primitive.Object.IsValid())
		{
			m_previousWorldTransforms[primitive.Draw.Source.GpuSceneSlot] =
			    PreviousWorldTransform{.Object = primitive.Object, .WorldMatrix = primitive.Draw.Transform.WorldMatrix};
		}
	}
}

void RenderScene::CommitJointMatrixContinuity(const RenderDeformationWork& deformation)
{
	std::size_t jointRangeIndex = 0u;
	for (auto history = m_jointMatrixHistory.begin(); history != m_jointMatrixHistory.end();)
	{
		while (jointRangeIndex < deformation.JointMatrixCopyRanges.size()
		    && deformation.JointMatrixCopyRanges[jointRangeIndex].Object < history->first)
		{
			++jointRangeIndex;
		}
		if (jointRangeIndex >= deformation.JointMatrixCopyRanges.size()
		    || deformation.JointMatrixCopyRanges[jointRangeIndex].Object != history->first)
		{
			history = m_jointMatrixHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderJointMatrixCopyRange& range : deformation.JointMatrixCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.Current.size();
		if (end <= deformation.JointMatrices.size())
		{
			std::vector<DirectX::XMFLOAT4X4>& history = m_jointMatrixHistory[range.Object];
			history.assign(deformation.JointMatrices.begin() + begin, deformation.JointMatrices.begin() + end);
		}
	}
}

void RenderScene::CommitMorphWeightContinuity(const RenderDeformationWork& deformation)
{
	const auto retainsMorphHistory = [&deformation](const RenderMorphWeightCopyRange& range) noexcept
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.TargetCount;
		if (end > deformation.MorphWeights.size())
		{
			return false;
		}
		const std::span<const float> weights{deformation.MorphWeights.data() + begin, range.TargetCount};
		return !range.Current.empty() || !std::all_of(weights.begin(), weights.end(), [](float weight) { return weight == 0.0f; });
	};

	std::size_t morphRangeIndex = 0u;
	for (auto history = m_morphWeightHistory.begin(); history != m_morphWeightHistory.end();)
	{
		while (morphRangeIndex < deformation.MorphWeightCopyRanges.size()
		    && deformation.MorphWeightCopyRanges[morphRangeIndex].Object < history->first)
		{
			++morphRangeIndex;
		}
		const bool retain = morphRangeIndex < deformation.MorphWeightCopyRanges.size()
		    && deformation.MorphWeightCopyRanges[morphRangeIndex].Object == history->first
		    && retainsMorphHistory(deformation.MorphWeightCopyRanges[morphRangeIndex]);
		if (!retain)
		{
			history = m_morphWeightHistory.erase(history);
		}
		else
		{
			++history;
		}
	}

	for (const RenderMorphWeightCopyRange& range : deformation.MorphWeightCopyRanges)
	{
		const std::size_t begin = range.OutputOffset;
		const std::size_t end = begin + range.TargetCount;
		if (end <= deformation.MorphWeights.size() && retainsMorphHistory(range))
		{
			std::vector<float>& history = m_morphWeightHistory[range.Object];
			history.assign(deformation.MorphWeights.begin() + begin, deformation.MorphWeights.begin() + end);
		}
	}
}

void RenderScene::ResetContinuity() noexcept
{
	m_previousWorldTransforms.clear();
	m_jointMatrixHistory.clear();
	m_morphWeightHistory.clear();
}

void RenderScene::PromoteResidentGpuMeshes() noexcept
{
	bool changed = false;
	for (RenderPrimitive& primitive : m_primitives)
	{
		if (primitive.HasPendingStatic)
		{
			if (!primitive.PendingGpuMesh)
			{
				primitive.PendingGpuMesh = m_gpuMeshCache->Request(primitive.PendingStatic.Mesh);
			}

			if (m_gpuMeshCache->Resolve(primitive.PendingGpuMesh) != nullptr)
			{
				primitive.Static = std::move(primitive.PendingStatic);
				primitive.GpuMesh = primitive.PendingGpuMesh;
				primitive.PendingGpuMesh = {};
				primitive.GpuMeshResident = true;
				primitive.HasPendingStatic = false;
				changed = true;
			}
			continue;
		}

		if (!primitive.GpuMesh)
		{
			primitive.GpuMesh = m_gpuMeshCache->Request(primitive.Static.Mesh);
		}

		if (!primitive.GpuMeshResident && m_gpuMeshCache->Resolve(primitive.GpuMesh) != nullptr)
		{
			primitive.GpuMeshResident = true;
			changed = true;
		}
	}

	if (changed)
	{
		++m_structuralRevision;
	}

	RetainReferencedGpuMeshes();
}

RenderSceneApplyStatus RenderScene::ApplyValidatedDelta(const RenderSceneDelta& delta, std::string& diagnostic)
{
	std::vector<GpuMeshHandle> createMeshes;
	std::vector<GpuMeshHandle> updateMeshes;
	ResolveGpuMeshes(delta, createMeshes, updateMeshes);

	if (delta.ResetScene)
	{
		for (const RenderPrimitive& primitive : m_primitives)
		{
			m_gpuSceneSlots->Retire(primitive.GpuSceneSlot);
		}
		m_primitives.clear();
		ResetContinuity();
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
	return RenderSceneApplyStatus::Applied;
}

bool RenderScene::ValidateDynamic(const RenderSceneDynamicData& dynamic, const RenderSceneDelta& delta, std::string& diagnostic) const
{
	if (!HasStrictlyOrderedDynamicObjects(dynamic.Objects) || !HasStrictlyOrderedJointMatrixRanges(dynamic.JointMatrixRanges)
	    || !HasStrictlyOrderedMorphWeightRanges(dynamic.MorphWeightRanges))
	{
		diagnostic = "Render-frame primitive updates are unordered or duplicated.";
		return false;
	}

	for (const RenderObjectDynamicData& primitive : dynamic.Objects)
	{
		if (!primitive.Object.IsValid() || !IsObjectAvailable(primitive.Object, delta))
		{
			diagnostic = "Render-frame primitive update is invalid.";
			return false;
		}
	}

	for (const RenderObjectCreate& create : delta.Creates)
	{
		const auto dynamicObject = std::lower_bound(
		    dynamic.Objects.begin(),
		    dynamic.Objects.end(),
		    create.Object,
		    [](const RenderObjectDynamicData& primitive, RenderObjectId identity) { return primitive.Object < identity; });
		const bool hasDynamicData = dynamicObject != dynamic.Objects.end() && dynamicObject->Object == create.Object;
		if (!hasDynamicData)
		{
			diagnostic = "Render scene create has no dynamic primitive data.";
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

void RenderScene::ApplyDynamic(RenderSceneDynamicData&& dynamic) noexcept
{
	for (const RenderObjectDynamicData& dynamicPrimitive : dynamic.Objects)
	{
		RenderPrimitive* primitive = FindMutable(dynamicPrimitive.Object);
		if (primitive != nullptr)
		{
			primitive->Dynamic = dynamicPrimitive;
		}
	}
	m_lights = std::move(dynamic.Lights);
	m_jointMatrixRanges = std::move(dynamic.JointMatrixRanges);
	m_jointMatrices = std::move(dynamic.JointMatrices);
	m_morphWeightRanges = std::move(dynamic.MorphWeightRanges);
	m_morphWeights = std::move(dynamic.MorphWeights);
}

RenderSceneApplyStatus RenderScene::ValidateDelta(const RenderSceneDelta& delta, std::string& diagnostic) const
{
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		diagnostic = "Render scene delta identity is invalid.";
		return RenderSceneApplyStatus::Rejected;
	}
	if (delta.SceneGeneration < m_sceneGeneration)
	{
		diagnostic = "Render scene delta is stale.";
		return RenderSceneApplyStatus::Stale;
	}
	if (delta.SceneGeneration == m_sceneGeneration && delta.SequenceNumber == m_sequenceNumber)
	{
		diagnostic = "Render scene delta is duplicated.";
		return RenderSceneApplyStatus::Duplicate;
	}
	if (delta.SceneGeneration == m_sceneGeneration && m_sequenceNumber != 0 && delta.SequenceNumber != m_sequenceNumber + 1)
	{
		diagnostic = "Render scene delta is out of order.";
		return RenderSceneApplyStatus::OutOfOrder;
	}
	if (delta.SceneGeneration > m_sceneGeneration && !delta.ResetScene)
	{
		diagnostic = "A new render scene generation requires a reset delta.";
		return RenderSceneApplyStatus::Rejected;
	}
	if (!HasOrderedDeltaObjects(delta))
	{
		diagnostic = "Render scene primitive changes are not deterministically ordered.";
		return RenderSceneApplyStatus::Rejected;
	}
	if (HasConflictingDeltaObjects(delta))
	{
		diagnostic = "Render scene primitive changes are duplicated or conflicting.";
		return RenderSceneApplyStatus::Rejected;
	}

	for (RenderObjectId primitiveId : delta.Destroys)
	{
		if (!primitiveId.IsValid() || delta.ResetScene || Find(primitiveId) == nullptr)
		{
			diagnostic = "Render scene destroy is invalid.";
			return RenderSceneApplyStatus::Rejected;
		}
	}
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Static.Mesh.IsValid() || (!delta.ResetScene && Find(create.Object) != nullptr))
		{
			diagnostic = "Render scene create is invalid.";
			return RenderSceneApplyStatus::Rejected;
		}
	}
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (!update.Object.IsValid() || !update.Static.Mesh.IsValid() || delta.ResetScene || Find(update.Object) == nullptr)
		{
			diagnostic = "Render scene update is invalid.";
			return RenderSceneApplyStatus::Rejected;
		}
	}
	diagnostic.clear();
	return RenderSceneApplyStatus::Applied;
}

void RenderScene::ApplyDestroys(const RenderSceneDelta& delta)
{
	for (RenderObjectId primitiveId : delta.Destroys)
	{
		const auto primitive = std::lower_bound(
		    m_primitives.begin(),
		    m_primitives.end(),
		    primitiveId,
		    [](const RenderPrimitive& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		if (primitive == m_primitives.end() || primitive->Object != primitiveId)
		{
			continue;
		}

		m_gpuSceneSlots->Retire(primitive->GpuSceneSlot);
		m_primitives.erase(primitive);
	}
}

void RenderScene::ResolveGpuMeshes(
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

void RenderScene::ApplyCreates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes)
{
	for (std::size_t index = 0; index < delta.Creates.size(); ++index)
	{
		const RenderObjectCreate& create = delta.Creates[index];
		const auto insertion = std::lower_bound(
		    m_primitives.begin(),
		    m_primitives.end(),
		    create.Object,
		    [](const RenderPrimitive& candidate, RenderObjectId identity) { return candidate.Object < identity; });
		m_primitives.insert(
		    insertion,
		    RenderPrimitive{
		        .Object = create.Object,
		        .Static = create.Static,
		        .GpuMesh = meshes[index],
		        .GpuSceneSlot = m_gpuSceneSlots->Allocate(),
		        .GpuMeshResident = m_gpuMeshCache->Resolve(meshes[index]) != nullptr});
	}
}

void RenderScene::ApplyUpdates(const RenderSceneDelta& delta, std::span<const GpuMeshHandle> meshes)
{
	for (std::size_t index = 0; index < delta.Updates.size(); ++index)
	{
		const RenderObjectUpdate& update = delta.Updates[index];
		RenderPrimitive* primitive = FindMutable(update.Object);
		if (primitive == nullptr)
		{
			continue;
		}

		if (m_gpuMeshCache->Resolve(meshes[index]) != nullptr)
		{
			primitive->Static = update.Static;
			primitive->GpuMesh = meshes[index];
			primitive->PendingStatic = {};
			primitive->PendingGpuMesh = {};
			primitive->GpuMeshResident = true;
			primitive->HasPendingStatic = false;
		}
		else
		{
			primitive->PendingStatic = update.Static;
			primitive->PendingGpuMesh = meshes[index];
			primitive->HasPendingStatic = true;
		}
	}
}

void RenderScene::PublishResources(const RenderSceneDelta& delta)
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

void RenderScene::RetainReferencedGpuMeshes() noexcept
{
	std::vector<GpuMeshHandle> handles;
	handles.reserve(m_primitives.size() * 2u);
	for (const RenderPrimitive& primitive : m_primitives)
	{
		if (primitive.GpuMesh)
		{
			handles.push_back(primitive.GpuMesh);
		}
		if (primitive.PendingGpuMesh)
		{
			handles.push_back(primitive.PendingGpuMesh);
		}
	}

	m_gpuMeshCache->RetainOnly(handles);
}

bool RenderScene::IsObjectAvailable(RenderObjectId primitiveId, const RenderSceneDelta& delta) const noexcept
{
	const auto created = std::lower_bound(
	    delta.Creates.begin(),
	    delta.Creates.end(),
	    primitiveId,
	    [](const RenderObjectCreate& create, RenderObjectId identity) { return create.Object < identity; });
	if (created != delta.Creates.end() && created->Object == primitiveId)
	{
		return true;
	}

	if (delta.ResetScene)
	{
		return false;
	}

	const bool destroyed = std::binary_search(delta.Destroys.begin(), delta.Destroys.end(), primitiveId);
	return !destroyed && Find(primitiveId) != nullptr;
}

bool RenderScene::HasOrderedDeltaObjects(const RenderSceneDelta& delta) noexcept
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

bool RenderScene::HasConflictingDeltaObjects(const RenderSceneDelta& delta) noexcept
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

bool RenderScene::HasStrictlyOrderedDynamicObjects(std::span<const RenderObjectDynamicData> primitives) noexcept
{
	for (std::size_t index = 1u; index < primitives.size(); ++index)
	{
		if (!(primitives[index - 1u].Object < primitives[index].Object))
		{
			return false;
		}
	}
	return true;
}

bool RenderScene::HasStrictlyOrderedJointMatrixRanges(std::span<const RenderJointMatrixRange> ranges) noexcept
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

bool RenderScene::HasStrictlyOrderedMorphWeightRanges(std::span<const RenderMorphWeightRange> ranges) noexcept
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

const RenderPrimitive* RenderScene::Find(RenderObjectId primitiveId) const noexcept
{
	const auto primitive = std::lower_bound(
	    m_primitives.begin(),
	    m_primitives.end(),
	    primitiveId,
	    [](const RenderPrimitive& candidate, RenderObjectId identity) { return candidate.Object < identity; });
	return primitive == m_primitives.end() || primitive->Object != primitiveId ? nullptr : &*primitive;
}

RenderPrimitive* RenderScene::FindMutable(RenderObjectId primitiveId) noexcept
{
	return const_cast<RenderPrimitive*>(std::as_const(*this).Find(primitiveId));
}
