#include "PCH.h"
#include "Scene/RenderScene.h"

#include "Meshes/GpuMeshCache.h"
#include "Scene/GpuScene/GpuSceneSlotAllocator.h"
#include "Scene/GpuScene/RenderGpuScene.h"
#include "Scene/RayTracing/RenderRayTracingScene.h"

#include <algorithm>
#include <utility>

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
		m_renderRayTracingScene->SynchronizeShaderTablePlan(m_primitives, m_materials);
	}

	RetainReferencedGpuMeshes();
}

void RenderScene::ApplyValidatedDelta(const RenderSceneDelta& delta)
{
	std::vector<GpuMeshHandle> createMeshes;
	std::vector<GpuMeshHandle> updateMeshes;
	ResolveGpuMeshes(delta, createMeshes, updateMeshes);

	if (delta.ResetScene)
	{
		m_renderGpuScene->Reset();
		m_renderRayTracingScene->Clear();
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
	if (delta.ResetScene || !delta.Creates.empty() || !delta.Updates.empty() || !delta.Destroys.empty() || delta.Materials)
	{
		m_renderRayTracingScene->SynchronizeShaderTablePlan(m_primitives, m_materials);
	}

	m_sceneGeneration = delta.SceneGeneration;
	m_sequenceNumber = delta.SequenceNumber;
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
