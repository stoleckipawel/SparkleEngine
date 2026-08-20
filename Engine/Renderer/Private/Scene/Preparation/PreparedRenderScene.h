#pragma once

#include "Renderer/Public/SceneData/DirectionalLight.h"
#include "Renderer/Public/SceneData/PointLight.h"
#include "Renderer/Public/SceneData/RectLight.h"
#include "Renderer/Public/SceneData/SpotLight.h"
#include "Scene/Materials/MaterialData.h"
#include "Scene/Preparation/RenderLightCollection.h"
#include "Scene/Preparation/RenderMeshWorldBounds.h"
#include "Scene/Preparation/RenderPrimitivePreparation.h"
#include "Scene/Preparation/RenderRayTracingWorkPlan.h"
#include "Scene/Preparation/RenderSkyData.h"
#include "RHI/Public/Descriptors/RhiDescriptorHandles.h"

#include <DirectXMath.h>

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

class RenderMaterialGeneration;
struct RenderSceneGpuBindings;

struct ResolvedMaterialTextureTable final
{
	RhiDescriptorTableBinding Binding = {};
	std::uint32_t DescriptorCount = 0u;
	std::uint64_t Generation = 0u;

	explicit operator bool() const noexcept;
};

struct PreparedRenderScene final
{
	std::uint64_t structuralRevision = 0;
	std::uint64_t materialRevision = 0;
	RenderLightCollection<DirectionalLight> directionalLights;
	RenderLightCollection<PointLight> pointLights;
	RenderLightCollection<SpotLight> spotLights;
	RenderLightCollection<RectLight> rectLights;
	RenderSkyData sky;
	std::vector<PreparedRenderPrimitive> primitives;
	std::vector<RenderMeshInstanceGroup> instanceGroups;
	std::vector<DirectX::XMFLOAT4X4> jointMatrices;
	std::vector<DirectX::XMFLOAT4X4> previousJointMatrices;
	std::vector<float> morphWeights;
	std::vector<float> previousMorphWeights;
	RenderRayTracingWorkPlan rayTracingWork;
	std::shared_ptr<const RenderMaterialGeneration> materialGeneration;
	std::span<const MaterialData> materials;
	ResolvedMaterialTextureTable materialTextureTable = {};
	// This borrows the selected frame-slot projection owned by RenderScene's GPU capability.
	const RenderSceneGpuBindings* gpuBindings = nullptr;

	void ResetForReuse() noexcept;
};
