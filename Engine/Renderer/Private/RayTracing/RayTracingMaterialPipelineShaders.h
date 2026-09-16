#pragma once

#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"
#include "Renderer/Private/RayTracing/RayTracingShaderFeatureFlags.h"

class RayTracingMaterialMiss final : public GlobalShader<RayTracingMaterialMiss>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::SceneBindings;
};

class RayTracingMaterialClosestHit final : public GlobalShader<RayTracingMaterialClosestHit>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::SceneBindings;
};

class RayTracingMaterialAnyHit final : public GlobalShader<RayTracingMaterialAnyHit>
{
public:
	static constexpr ShaderFeatureFlags kShaderFeatures = RayTracingShaderFeatureFlags::SceneBindings;
};
