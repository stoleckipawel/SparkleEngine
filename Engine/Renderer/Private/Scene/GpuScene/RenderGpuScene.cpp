#include "PCH.h"
#include "Scene/GpuScene/RenderGpuScene.h"

#include "RHI/Public/Frame/RhiFrameConstants.h"
#include "Scene/GpuScene/PersistentStructuredBuffer.h"
#include "Scene/GpuScene/RenderGpuGeometryState.h"
#include "Scene/GpuScene/RenderGpuLightingPayloadBuilder.h"
#include "Scene/GpuScene/RenderGpuRayTracingPayloadBuilder.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"
#include "Scene/GpuScene/RenderSceneGpuBindings.h"

#include <array>
#include <limits>
#include <span>

struct RenderGpuDynamicFrameStorage final
{
	void Reset() noexcept;

	PersistentStructuredBuffer DirectionalLights;
	PersistentStructuredBuffer PointLights;
	PersistentStructuredBuffer SpotLights;
	PersistentStructuredBuffer RectLights;
	PersistentStructuredBuffer MeshInstances;
	PersistentStructuredBuffer MeshInstanceSlots;
	PersistentStructuredBuffer JointMatrices;
	PersistentStructuredBuffer PreviousJointMatrices;
	PersistentStructuredBuffer MorphWeights;
	PersistentStructuredBuffer PreviousMorphWeights;
	PersistentStructuredBuffer RayTracingInstances;
	PersistentStructuredBuffer RayTracingMaterials;
	RenderSceneGpuBindings Bindings;
	std::uint64_t MeshInstanceRevision = 0u;
	std::uint64_t MeshInstanceSlotRevision = 0u;
	std::uint64_t RayTracingPayloadRevision = 0u;
};

struct RenderGpuRayTracingStorage final
{
	void Reset() noexcept;

	PersistentStructuredBuffer Vertices;
	PersistentStructuredBuffer SkinInfluences;
	PersistentStructuredBuffer MorphTargetDeltas;
	PersistentStructuredBuffer Indices;
};

void RenderGpuDynamicFrameStorage::Reset() noexcept
{
	DirectionalLights.Reset();
	PointLights.Reset();
	SpotLights.Reset();
	RectLights.Reset();
	MeshInstances.Reset();
	MeshInstanceSlots.Reset();
	JointMatrices.Reset();
	PreviousJointMatrices.Reset();
	MorphWeights.Reset();
	PreviousMorphWeights.Reset();
	RayTracingInstances.Reset();
	RayTracingMaterials.Reset();
	Bindings = {};
	MeshInstanceRevision = 0u;
	MeshInstanceSlotRevision = 0u;
	RayTracingPayloadRevision = 0u;
}

void RenderGpuRayTracingStorage::Reset() noexcept
{
	Vertices.Reset();
	SkinInfluences.Reset();
	MorphTargetDeltas.Reset();
	Indices.Reset();
}

struct RenderGpuScene::Impl final
{
	Impl(RhiResourceService& resourceService, const GpuMeshCache& meshes) noexcept :
	    ResourceService(&resourceService),
	    Meshes(&meshes)
	{
	}

	const RenderSceneGpuBindings& Update(const PreparedRenderScene& preparedScene, const RenderView& view, std::uint32_t frameIndex)
	{
		RenderGpuDynamicFrameStorage& dynamicStorage = DynamicFrames[frameIndex % DynamicFrames.size()];
		Geometry.Update(preparedScene, view);

		UpdateLighting(preparedScene, dynamicStorage);
		UpdateGeometry(dynamicStorage);
		UpdateRayTracing(preparedScene, dynamicStorage);
		return dynamicStorage.Bindings;
	}

	void UpdateLighting(const PreparedRenderScene& preparedScene, RenderGpuDynamicFrameStorage& storage)
	{
		RenderGpuLightingPayloadBuilder::Build(preparedScene, LightingPayloads);

		storage.DirectionalLights.Update(*ResourceService, std::span{LightingPayloads.DirectionalLights}, L"DirectionalLights");
		storage.PointLights.Update(*ResourceService, std::span{LightingPayloads.PointLights}, L"PointLights");
		storage.SpotLights.Update(*ResourceService, std::span{LightingPayloads.SpotLights}, L"SpotLights");
		storage.RectLights.Update(*ResourceService, std::span{LightingPayloads.RectLights}, L"RectLights");

		storage.Bindings.Lighting = RenderSceneGpuLightingBindings{
		    .Uniform = LightingPayloads.Uniform,
		    .DirectionalLights = storage.DirectionalLights.GetBinding(),
		    .PointLights = storage.PointLights.GetBinding(),
		    .SpotLights = storage.SpotLights.GetBinding(),
		    .RectLights = storage.RectLights.GetBinding()};
	}

	void UpdateGeometry(RenderGpuDynamicFrameStorage& storage)
	{
		const RenderGpuGeometryPayloads& payloads = Geometry.GetPayloads();
		Geometry.CollectMeshInstanceWriteRanges(storage.MeshInstanceRevision, MeshInstanceWriteRanges);
		storage.MeshInstances.UpdateRanges(
		    *ResourceService,
		    std::as_bytes(std::span<const MeshInstanceData>{payloads.MeshInstances}),
		    static_cast<std::uint32_t>(sizeof(MeshInstanceData)),
		    MeshInstanceWriteRanges,
		    L"MeshInstances");
		storage.MeshInstanceRevision = Geometry.GetMeshInstanceRevision();

		if (storage.MeshInstanceSlotRevision != Geometry.GetMeshInstanceSlotRevision())
		{
			storage.MeshInstanceSlots.Update(*ResourceService, std::span{payloads.MeshInstanceSlots}, L"MeshInstanceSlots");
			storage.MeshInstanceSlotRevision = Geometry.GetMeshInstanceSlotRevision();
		}

		storage.JointMatrices.Update(*ResourceService, std::span{payloads.JointMatrices}, L"SkinningJointMatrices");
		storage.PreviousJointMatrices.Update(*ResourceService, std::span{payloads.PreviousJointMatrices}, L"PreviousSkinningJointMatrices");
		storage.MorphWeights.Update(*ResourceService, std::span{payloads.MorphWeights}, L"MorphWeights");
		storage.PreviousMorphWeights.Update(*ResourceService, std::span{payloads.PreviousMorphWeights}, L"PreviousMorphWeights");

		storage.Bindings.Geometry = RenderSceneGpuGeometryBindings{
		    .MeshInstances = storage.MeshInstances.GetBinding(),
		    .MeshInstanceSlots = storage.MeshInstanceSlots.GetBinding(),
		    .JointMatrices = storage.JointMatrices.GetBinding(),
		    .PreviousJointMatrices = storage.PreviousJointMatrices.GetBinding(),
		    .MorphWeights = storage.MorphWeights.GetBinding(),
		    .PreviousMorphWeights = storage.PreviousMorphWeights.GetBinding()};
	}

	void UpdateRayTracing(const PreparedRenderScene& preparedScene, RenderGpuDynamicFrameStorage& storage)
	{
		const std::uint64_t textureGeneration = preparedScene.materialTextureTable.Generation;
		const bool topologyChanged = preparedScene.structuralRevision != RayTracingStructuralRevision;
		const bool payloadChanged = topologyChanged || preparedScene.materialRevision != RayTracingMaterialRevision
		    || textureGeneration != RayTracingTextureGeneration;
		if (payloadChanged)
		{
			RenderGpuRayTracingPayloadBuilder::Build(preparedScene, *Meshes, RayTracingPayloads);
			if (topologyChanged || RayTracingPayloads.InstanceCount == 0u || !RayTracing.Vertices.GetBinding()
			    || !RayTracing.MorphTargetDeltas.GetBinding())
			{
				UpdateRayTracingTopology();
			}

			RayTracingStructuralRevision = preparedScene.structuralRevision;
			RayTracingMaterialRevision = preparedScene.materialRevision;
			RayTracingTextureGeneration = textureGeneration;
			++RayTracingPayloadRevision;
		}

		if (storage.RayTracingPayloadRevision != RayTracingPayloadRevision)
		{
			storage.RayTracingInstances.Update(*ResourceService, std::span{RayTracingPayloads.Instances}, L"RayTracingHitInstances");
			storage.RayTracingMaterials.Update(*ResourceService, std::span{RayTracingPayloads.Materials}, L"RayTracingHitMaterials");
			storage.RayTracingPayloadRevision = RayTracingPayloadRevision;
		}

		storage.Bindings.RayTracing = RenderSceneGpuRayTracingBindings{
		    .Vertices = RayTracing.Vertices.GetBinding(),
		    .SkinInfluences = RayTracing.SkinInfluences.GetBinding(),
		    .MorphTargetDeltas = RayTracing.MorphTargetDeltas.GetBinding(),
		    .Indices = RayTracing.Indices.GetBinding(),
		    .Instances = storage.RayTracingInstances.GetBinding(),
		    .Materials = storage.RayTracingMaterials.GetBinding(),
		    .InstanceCount = RayTracingPayloads.InstanceCount,
		    .MaterialCount = RayTracingPayloads.MaterialCount};
	}

	void UpdateRayTracingTopology()
	{
		RayTracing.Vertices.Replace(*ResourceService, std::span{RayTracingPayloads.Vertices}, L"RayTracingHitVertices");
		RayTracing.SkinInfluences.Replace(*ResourceService, std::span{RayTracingPayloads.SkinInfluences}, L"RayTracingHitSkinInfluences");
		RayTracing.MorphTargetDeltas.Replace(
		    *ResourceService,
		    std::span{RayTracingPayloads.MorphTargetDeltas},
		    L"RayTracingHitMorphTargetDeltas");
		RayTracing.Indices.Replace(*ResourceService, std::span{RayTracingPayloads.Indices}, L"RayTracingHitIndices");
	}

	void Reset() noexcept
	{
		for (RenderGpuDynamicFrameStorage& storage : DynamicFrames)
		{
			storage.Reset();
		}
		RayTracing.Reset();
		Geometry.Reset();
		LightingPayloads = {};
		RayTracingPayloads = {};
		RayTracingPayloadRevision = 0u;
		RayTracingStructuralRevision = (std::numeric_limits<std::uint64_t>::max)();
		RayTracingMaterialRevision = (std::numeric_limits<std::uint64_t>::max)();
		RayTracingTextureGeneration = (std::numeric_limits<std::uint64_t>::max)();
	}

	RhiResourceService* ResourceService = nullptr;
	const GpuMeshCache* Meshes = nullptr;
	// RHI frame-slot reuse waits for the slot's submission token, so a borrowed binding projection remains stable through execution.
	std::array<RenderGpuDynamicFrameStorage, RhiFrameConstants::MaxFrameSlotCount> DynamicFrames;
	RenderGpuRayTracingStorage RayTracing;
	RenderGpuGeometryState Geometry;
	RenderGpuLightingPayloads LightingPayloads;
	std::vector<StructuredBufferElementRange> MeshInstanceWriteRanges;
	RenderGpuRayTracingPayloads RayTracingPayloads;
	std::uint64_t RayTracingPayloadRevision = 0u;
	std::uint64_t RayTracingStructuralRevision = (std::numeric_limits<std::uint64_t>::max)();
	std::uint64_t RayTracingMaterialRevision = (std::numeric_limits<std::uint64_t>::max)();
	std::uint64_t RayTracingTextureGeneration = (std::numeric_limits<std::uint64_t>::max)();
};

RenderGpuScene::RenderGpuScene(RhiResourceService& resourceService, const GpuMeshCache& meshes) :
    m_impl(std::make_unique<Impl>(resourceService, meshes))
{
}

RenderGpuScene::~RenderGpuScene() noexcept = default;

const RenderSceneGpuBindings& RenderGpuScene::Update(
    const PreparedRenderScene& preparedScene,
    const RenderView& view,
    std::uint32_t frameIndex)
{
	return m_impl->Update(preparedScene, view, frameIndex);
}

void RenderGpuScene::Reset() noexcept
{
	m_impl->Reset();
}
