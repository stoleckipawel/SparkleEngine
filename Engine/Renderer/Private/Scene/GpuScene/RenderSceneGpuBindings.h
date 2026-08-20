#pragma once

#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"

#include <cstdint>

struct RenderSceneGpuBufferBinding final
{
	RhiOwnedResourceHandle Resource = {};
	std::uint64_t SizeInBytes = 0;
	std::uint32_t StrideInBytes = 0;

	bool IsValid() const noexcept;
	explicit operator bool() const noexcept;
};

struct RenderSceneGpuLightingBindings final
{
	SceneLightingUniformData Uniform = {};
	RenderSceneGpuBufferBinding DirectionalLights;
	RenderSceneGpuBufferBinding PointLights;
	RenderSceneGpuBufferBinding SpotLights;
	RenderSceneGpuBufferBinding RectLights;
};

struct RenderSceneGpuGeometryBindings final
{
	RenderSceneGpuBufferBinding MeshInstances;
	RenderSceneGpuBufferBinding MeshInstanceSlots;
	RenderSceneGpuBufferBinding JointMatrices;
	RenderSceneGpuBufferBinding PreviousJointMatrices;
	RenderSceneGpuBufferBinding MorphWeights;
	RenderSceneGpuBufferBinding PreviousMorphWeights;

	bool HasMeshInstanceBuffers() const noexcept;
	bool HasSkinningBuffers() const noexcept;
	bool HasMorphingBuffers() const noexcept;
};

struct RenderSceneGpuRayTracingBindings final
{
	RenderSceneGpuBufferBinding Vertices;
	RenderSceneGpuBufferBinding SkinInfluences;
	RenderSceneGpuBufferBinding MorphTargetDeltas;
	RenderSceneGpuBufferBinding Indices;
	RenderSceneGpuBufferBinding Instances;
	RenderSceneGpuBufferBinding Materials;
	std::uint32_t InstanceCount = 0u;
	std::uint32_t MaterialCount = 0u;

	bool HasCompleteBuffers() const noexcept;
};

struct RenderSceneGpuBindings final
{
	RenderSceneGpuLightingBindings Lighting = {};
	RenderSceneGpuGeometryBindings Geometry = {};
	RenderSceneGpuRayTracingBindings RayTracing = {};
};
