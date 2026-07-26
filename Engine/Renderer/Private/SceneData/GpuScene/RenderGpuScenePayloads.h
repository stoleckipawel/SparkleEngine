#pragma once

#include "RayTracing/RayTracingHitData.h"
#include "ShaderData/MeshInstanceShaderData.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"

#include <cstdint>
#include <vector>

struct RenderGpuLightingPayloads final
{
	ViewLightingData Constants = {};
	std::vector<DirectionalLightConstantBufferData> DirectionalLights;
	std::vector<PointLightConstantBufferData> PointLights;
	std::vector<SpotLightConstantBufferData> SpotLights;
	std::vector<RectLightConstantBufferData> RectLights;
};

struct RenderGpuGeometryPayloads final
{
	std::vector<MeshInstanceData> MeshInstances;
	std::vector<std::uint32_t> MeshInstanceSlots;
	std::vector<JointMatrixData> JointMatrices;
	std::vector<JointMatrixData> PreviousJointMatrices;
};

struct RenderGpuRayTracingPayloads final
{
	std::vector<RayTracingHitVertex> Vertices;
	std::vector<VertexSkinInfluenceData> SkinInfluences;
	std::vector<std::uint32_t> Indices;
	std::vector<RayTracingHitInstance> Instances;
	std::vector<RayTracingHitMaterial> Materials;
	std::uint32_t InstanceCount = 0;
	std::uint32_t MaterialCount = 0;
};
