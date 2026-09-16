#include "PCH.h"

#include "RayTracing/RayTracingMaterialPipelineShaders.h"

IMPLEMENT_GLOBAL_SHADER(RayTracingMaterialMiss, "/Engine/RayTracing/RayTracingMaterialPipeline.hlsl", "RayTracingMaterialMiss", Miss);
IMPLEMENT_GLOBAL_SHADER(
    RayTracingMaterialClosestHit,
    "/Engine/RayTracing/RayTracingMaterialPipeline.hlsl",
    "RayTracingMaterialClosestHit",
    ClosestHit);
IMPLEMENT_GLOBAL_SHADER(RayTracingMaterialAnyHit, "/Engine/RayTracing/RayTracingMaterialPipeline.hlsl", "RayTracingMaterialAnyHit", AnyHit);
