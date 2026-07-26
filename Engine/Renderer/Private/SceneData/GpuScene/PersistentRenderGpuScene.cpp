#include "PCH.h"
#include "SceneData/GpuScene/PersistentRenderGpuScene.h"

#include "RHI/Public/Frame/RhiFrameConstants.h"
#include "SceneData/GpuScene/PersistentStructuredBuffer.h"
#include "SceneData/GpuScene/RenderGpuScenePayloadBuilder.h"
#include "SceneData/RenderSceneData.h"
#include "SceneData/RenderSceneGpuData.h"

#include <array>
#include <limits>
#include <span>

struct RenderGpuDynamicFrameStorage final
{
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
};

struct RenderGpuRayTracingStorage final
{
	PersistentStructuredBuffer Vertices;
	PersistentStructuredBuffer SkinInfluences;
	PersistentStructuredBuffer MorphTargetDeltas;
	PersistentStructuredBuffer Indices;
};

class PersistentRenderGpuSceneOperations final
{
  public:
	template <typename TValue>
	static bool Update(
	    PersistentStructuredBuffer& buffer,
	    RhiResourceService& resourceService,
	    const std::vector<TValue>& values,
	    std::wstring_view debugName)
	{
		return buffer.Update(
		    resourceService,
		    std::as_bytes(std::span<const TValue>{values}),
		    static_cast<std::uint32_t>(sizeof(TValue)),
		    debugName);
	}

	template <typename TValue>
	static bool Replace(
	    PersistentStructuredBuffer& buffer,
	    RhiResourceService& resourceService,
	    const std::vector<TValue>& values,
	    std::wstring_view debugName)
	{
		return buffer.Replace(
		    resourceService,
		    std::as_bytes(std::span<const TValue>{values}),
		    static_cast<std::uint32_t>(sizeof(TValue)),
		    debugName);
	}

	static void Reset(RenderGpuDynamicFrameStorage& storage) noexcept
	{
		storage.DirectionalLights.Reset();
		storage.PointLights.Reset();
		storage.SpotLights.Reset();
		storage.RectLights.Reset();
		storage.MeshInstances.Reset();
		storage.MeshInstanceSlots.Reset();
		storage.JointMatrices.Reset();
		storage.PreviousJointMatrices.Reset();
		storage.MorphWeights.Reset();
		storage.PreviousMorphWeights.Reset();
		storage.RayTracingInstances.Reset();
		storage.RayTracingMaterials.Reset();
		storage.Data = {};
	}

	static void Reset(RenderGpuRayTracingStorage& storage) noexcept
	{
		storage.Vertices.Reset();
		storage.SkinInfluences.Reset();
		storage.MorphTargetDeltas.Reset();
		storage.Indices.Reset();
	}
};

struct PersistentRenderGpuScene::Impl final
{
	Impl(
	    RhiResourceService& resourceService,
	    const GPUMeshCache& meshes) noexcept :
		ResourceService(&resourceService),
		Meshes(&meshes)
	{
	}

	const RenderSceneGpuData& Update(
	    const RenderSceneData& sceneData,
	    std::uint32_t frameIndex)
	{
		RenderGpuDynamicFrameStorage& dynamicStorage =
		    DynamicFrames[
		        frameIndex % RhiFrameConstants::FramesInFlight];
		UpdateLighting(sceneData, dynamicStorage);
		UpdateGeometry(sceneData, dynamicStorage);
		UpdateRayTracing(sceneData, dynamicStorage);
		return dynamicStorage.Data;
	}

	void UpdateLighting(
	    const RenderSceneData& sceneData,
	    RenderGpuDynamicFrameStorage& storage)
	{
		const RenderGpuLightingPayloads payloads =
		    RenderGpuScenePayloadBuilder::BuildLighting(sceneData);
		PersistentRenderGpuSceneOperations::Update(
		    storage.DirectionalLights,
		    *ResourceService,
		    payloads.DirectionalLights,
		    L"DirectionalLights");
		PersistentRenderGpuSceneOperations::Update(
		    storage.PointLights,
		    *ResourceService,
		    payloads.PointLights,
		    L"PointLights");
		PersistentRenderGpuSceneOperations::Update(
		    storage.SpotLights,
		    *ResourceService,
		    payloads.SpotLights,
		    L"SpotLights");
		PersistentRenderGpuSceneOperations::Update(
		    storage.RectLights,
		    *ResourceService,
		    payloads.RectLights,
		    L"RectLights");

		storage.Data.Lighting = RenderSceneGpuLightingData{
		    .Constants = payloads.Constants,
		    .DirectionalLights =
		        storage.DirectionalLights.GetBinding(),
		    .PointLights = storage.PointLights.GetBinding(),
		    .SpotLights = storage.SpotLights.GetBinding(),
		    .RectLights = storage.RectLights.GetBinding()};
	}

	void UpdateGeometry(
	    const RenderSceneData& sceneData,
	    RenderGpuDynamicFrameStorage& storage)
	{
		const RenderGpuGeometryPayloads payloads =
		    RenderGpuScenePayloadBuilder::BuildGeometry(sceneData);
		PersistentRenderGpuSceneOperations::Update(
		    storage.MeshInstances,
		    *ResourceService,
		    payloads.MeshInstances,
		    L"MeshInstances");
		PersistentRenderGpuSceneOperations::Update(
		    storage.MeshInstanceSlots,
		    *ResourceService,
		    payloads.MeshInstanceSlots,
		    L"MeshInstanceSlots");
		PersistentRenderGpuSceneOperations::Update(
		    storage.JointMatrices,
		    *ResourceService,
		    payloads.JointMatrices,
		    L"SkinningJointMatrices");
		PersistentRenderGpuSceneOperations::Update(
		    storage.PreviousJointMatrices,
		    *ResourceService,
		    payloads.PreviousJointMatrices,
		    L"PreviousSkinningJointMatrices");
		PersistentRenderGpuSceneOperations::Update(
		    storage.MorphWeights,
		    *ResourceService,
		    payloads.MorphWeights,
		    L"MorphWeights");
		PersistentRenderGpuSceneOperations::Update(
		    storage.PreviousMorphWeights,
		    *ResourceService,
		    payloads.PreviousMorphWeights,
		    L"PreviousMorphWeights");

		storage.Data.Geometry = RenderSceneGpuGeometryData{
		    .MeshInstances =
		        storage.MeshInstances.GetBinding(),
		    .MeshInstanceSlots =
		        storage.MeshInstanceSlots.GetBinding(),
		    .JointMatrices =
		        storage.JointMatrices.GetBinding(),
		    .PreviousJointMatrices =
		        storage.PreviousJointMatrices.GetBinding(),
		    .MorphWeights =
		        storage.MorphWeights.GetBinding(),
		    .PreviousMorphWeights =
		        storage.PreviousMorphWeights.GetBinding()};
	}

	void UpdateRayTracing(
	    const RenderSceneData& sceneData,
	    RenderGpuDynamicFrameStorage& storage)
	{
		const std::uint64_t textureGeneration =
		    sceneData.materialTextureTable.Generation;
		const bool topologyChanged =
		    sceneData.structuralRevision !=
		    RayTracingStructuralRevision;
		const bool payloadChanged =
		    topologyChanged ||
		    sceneData.materialRevision !=
		        RayTracingMaterialRevision ||
		    textureGeneration != RayTracingTextureGeneration;
		if (payloadChanged)
		{
			RayTracingPayloads =
			    RenderGpuScenePayloadBuilder::BuildRayTracing(
		        sceneData,
		        *Meshes);
			if (topologyChanged ||
			    RayTracingPayloads.InstanceCount == 0u ||
			    !RayTracing.Vertices.GetBinding() ||
			    !RayTracing.MorphTargetDeltas.GetBinding())
			{
				UpdateRayTracingTopology();
			}

			RayTracingStructuralRevision =
			    sceneData.structuralRevision;
			RayTracingMaterialRevision =
			    sceneData.materialRevision;
			RayTracingTextureGeneration = textureGeneration;
		}

		PersistentRenderGpuSceneOperations::Update(
		    storage.RayTracingInstances,
		    *ResourceService,
		    RayTracingPayloads.Instances,
		    L"RayTracingHitInstances");
		PersistentRenderGpuSceneOperations::Update(
		    storage.RayTracingMaterials,
		    *ResourceService,
		    RayTracingPayloads.Materials,
		    L"RayTracingHitMaterials");
		storage.Data.RayTracing =
		    RayTracingPayloads.InstanceCount == 0u ||
		            RayTracingPayloads.MaterialCount == 0u
		        ? RenderSceneGpuRayTracingData{}
		        : RenderSceneGpuRayTracingData{
		              .Vertices =
		                  RayTracing.Vertices.GetBinding(),
		              .SkinInfluences =
		                  RayTracing.SkinInfluences.GetBinding(),
		              .MorphTargetDeltas =
		                  RayTracing.MorphTargetDeltas.GetBinding(),
		              .Indices =
		                  RayTracing.Indices.GetBinding(),
		              .Instances =
		                  storage.RayTracingInstances.GetBinding(),
		              .Materials =
		                  storage.RayTracingMaterials.GetBinding(),
		              .InstanceCount =
		                  RayTracingPayloads.InstanceCount,
		              .MaterialCount =
		                  RayTracingPayloads.MaterialCount};
	}

	void UpdateRayTracingTopology()
	{
		if (RayTracingPayloads.InstanceCount == 0u ||
		    RayTracingPayloads.MaterialCount == 0u)
		{
			PersistentRenderGpuSceneOperations::Reset(RayTracing);
			return;
		}

		PersistentRenderGpuSceneOperations::Replace(
		    RayTracing.Vertices,
		    *ResourceService,
		    RayTracingPayloads.Vertices,
		    L"RayTracingHitVertices");
		PersistentRenderGpuSceneOperations::Replace(
		    RayTracing.SkinInfluences,
		    *ResourceService,
		    RayTracingPayloads.SkinInfluences,
		    L"RayTracingHitSkinInfluences");
		PersistentRenderGpuSceneOperations::Replace(
		    RayTracing.MorphTargetDeltas,
		    *ResourceService,
		    RayTracingPayloads.MorphTargetDeltas,
		    L"RayTracingHitMorphTargetDeltas");
		PersistentRenderGpuSceneOperations::Replace(
		    RayTracing.Indices,
		    *ResourceService,
		    RayTracingPayloads.Indices,
		    L"RayTracingHitIndices");
	}

	void Reset() noexcept
	{
		for (RenderGpuDynamicFrameStorage& storage :
		     DynamicFrames)
		{
			PersistentRenderGpuSceneOperations::Reset(storage);
		}
		PersistentRenderGpuSceneOperations::Reset(RayTracing);
		RayTracingPayloads = {};
		RayTracingStructuralRevision =
		    (std::numeric_limits<std::uint64_t>::max)();
		RayTracingMaterialRevision =
		    (std::numeric_limits<std::uint64_t>::max)();
		RayTracingTextureGeneration =
		    (std::numeric_limits<std::uint64_t>::max)();
	}

	RhiResourceService* ResourceService = nullptr;
	const GPUMeshCache* Meshes = nullptr;
	std::array<
	    RenderGpuDynamicFrameStorage,
	    RhiFrameConstants::FramesInFlight>
	    DynamicFrames;
	RenderGpuRayTracingStorage RayTracing;
	RenderGpuRayTracingPayloads RayTracingPayloads;
	std::uint64_t RayTracingStructuralRevision =
	    (std::numeric_limits<std::uint64_t>::max)();
	std::uint64_t RayTracingMaterialRevision =
	    (std::numeric_limits<std::uint64_t>::max)();
	std::uint64_t RayTracingTextureGeneration =
	    (std::numeric_limits<std::uint64_t>::max)();
};

PersistentRenderGpuScene::PersistentRenderGpuScene(
    RhiResourceService& resourceService,
    const GPUMeshCache& meshes) :
	m_impl(std::make_unique<Impl>(resourceService, meshes))
{
}

PersistentRenderGpuScene::~PersistentRenderGpuScene() noexcept =
    default;

const RenderSceneGpuData& PersistentRenderGpuScene::Update(
    const RenderSceneData& sceneData,
    std::uint32_t frameIndex)
{
	return m_impl->Update(sceneData, frameIndex);
}

void PersistentRenderGpuScene::Reset() noexcept
{
	m_impl->Reset();
}
