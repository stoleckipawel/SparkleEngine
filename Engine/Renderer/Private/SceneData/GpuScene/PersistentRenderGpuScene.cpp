#include "PCH.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"

#include "RHI/Public/Frame/RhiFrameConstants.h"
#include "SceneData/GpuScene/PersistentStructuredBuffer.h"
#include "SceneData/GpuScene/RenderGpuGeometryState.h"
#include "SceneData/GpuScene/RenderGpuLightingPayloadBuilder.h"
#include "SceneData/GpuScene/RenderGpuRayTracingPayloadBuilder.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderSceneGpuData.h"

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
	RenderSceneGpuData Data;
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
	Data = {};
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

struct PersistentRenderGpuScene::Impl final
{
	Impl(RhiResourceService& resourceService, const GpuMeshCache& meshes) noexcept :
	    ResourceService(&resourceService),
	    Meshes(&meshes)
	{
	}

	const RenderSceneGpuData& Update(const RenderSceneData& sceneData, std::uint32_t frameIndex)
	{
		RenderGpuDynamicFrameStorage& dynamicStorage = DynamicFrames[frameIndex % DynamicFrames.size()];
		Geometry.Update(sceneData);

		UpdateLighting(sceneData, dynamicStorage);
		UpdateGeometry(dynamicStorage);
		UpdateRayTracing(sceneData, dynamicStorage);
		return dynamicStorage.Data;
	}

	void UpdateLighting(const RenderSceneData& sceneData, RenderGpuDynamicFrameStorage& storage)
	{
		RenderGpuLightingPayloadBuilder::Build(sceneData, LightingPayloads);

		storage.DirectionalLights.Update(*ResourceService, std::span{LightingPayloads.DirectionalLights}, L"DirectionalLights");
		storage.PointLights.Update(*ResourceService, std::span{LightingPayloads.PointLights}, L"PointLights");
		storage.SpotLights.Update(*ResourceService, std::span{LightingPayloads.SpotLights}, L"SpotLights");
		storage.RectLights.Update(*ResourceService, std::span{LightingPayloads.RectLights}, L"RectLights");

		storage.Data.Lighting = RenderSceneGpuLightingData{
		    .Constants = LightingPayloads.Constants,
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

		storage.Data.Geometry = RenderSceneGpuGeometryData{
		    .MeshInstances = storage.MeshInstances.GetBinding(),
		    .MeshInstanceSlots = storage.MeshInstanceSlots.GetBinding(),
		    .JointMatrices = storage.JointMatrices.GetBinding(),
		    .PreviousJointMatrices = storage.PreviousJointMatrices.GetBinding(),
		    .MorphWeights = storage.MorphWeights.GetBinding(),
		    .PreviousMorphWeights = storage.PreviousMorphWeights.GetBinding()};
	}

	void UpdateRayTracing(const RenderSceneData& sceneData, RenderGpuDynamicFrameStorage& storage)
	{
		const std::uint64_t textureGeneration = sceneData.materialTextureTable.Generation;
		const bool topologyChanged = sceneData.structuralRevision != RayTracingStructuralRevision;
		const bool payloadChanged =
		    topologyChanged || sceneData.materialRevision != RayTracingMaterialRevision || textureGeneration != RayTracingTextureGeneration;
		if (payloadChanged)
		{
			RenderGpuRayTracingPayloadBuilder::Build(sceneData, *Meshes, RayTracingPayloads);
			if (topologyChanged || RayTracingPayloads.InstanceCount == 0u || !RayTracing.Vertices.GetBinding()
			    || !RayTracing.MorphTargetDeltas.GetBinding())
			{
				UpdateRayTracingTopology();
			}

			RayTracingStructuralRevision = sceneData.structuralRevision;
			RayTracingMaterialRevision = sceneData.materialRevision;
			RayTracingTextureGeneration = textureGeneration;
			++RayTracingPayloadRevision;
		}

		if (storage.RayTracingPayloadRevision != RayTracingPayloadRevision)
		{
			storage.RayTracingInstances.Update(*ResourceService, std::span{RayTracingPayloads.Instances}, L"RayTracingHitInstances");
			storage.RayTracingMaterials.Update(*ResourceService, std::span{RayTracingPayloads.Materials}, L"RayTracingHitMaterials");
			storage.RayTracingPayloadRevision = RayTracingPayloadRevision;
		}

		storage.Data.RayTracing = RenderSceneGpuRayTracingData{
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

PersistentRenderGpuScene::PersistentRenderGpuScene(RhiResourceService& resourceService, const GpuMeshCache& meshes) :
    m_impl(std::make_unique<Impl>(resourceService, meshes))
{
}

PersistentRenderGpuScene::~PersistentRenderGpuScene() noexcept = default;

const RenderSceneGpuData& PersistentRenderGpuScene::Update(const RenderSceneData& sceneData, std::uint32_t frameIndex)
{
	return m_impl->Update(sceneData, frameIndex);
}

void PersistentRenderGpuScene::Reset() noexcept
{
	m_impl->Reset();
}
