#pragma once

#include "RayTracing/RayTracingHitData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/MorphTargetShaderData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

#include <cstdint>
#include <vector>

struct RenderGpuLightingPayloads final
{
	SceneLightingUniformData Uniform = {};
	std::vector<DirectionalLightGpuData> DirectionalLights;
	std::vector<PointLightGpuData> PointLights;
	std::vector<SpotLightGpuData> SpotLights;
	std::vector<RectLightGpuData> RectLights;
};

struct RenderGpuGeometryPayloads final
{
	std::vector<MeshInstanceData> MeshInstances;
	std::vector<std::uint32_t> MeshInstanceSlots;
	std::vector<JointMatrixData> JointMatrices;
	std::vector<JointMatrixData> PreviousJointMatrices;
	std::vector<float> MorphWeights;
	std::vector<float> PreviousMorphWeights;
};

struct RenderGpuRayTracingPayloads final
{
	std::vector<RayTracingHitVertex> Vertices;
	std::vector<VertexSkinInfluenceData> SkinInfluences;
	std::vector<MorphTargetDeltaData> MorphTargetDeltas;
	std::vector<std::uint32_t> Indices;
	std::vector<RayTracingHitInstance> Instances;
	std::vector<RayTracingHitMaterial> Materials;
	std::uint32_t InstanceCount = 0;
	std::uint32_t MaterialCount = 0;
};
