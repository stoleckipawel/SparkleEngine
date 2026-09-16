#include "PCH.h"

#include "Passes/RayTracing/RayTracingGBufferShaders.h"

IMPLEMENT_GLOBAL_SHADER(
    RayTracingGBufferInlineCS,
    "/Engine/Passes/RayTracing/RayTracingGBufferInline.hlsl",
    "RayTracingGBufferInline",
    Compute);
IMPLEMENT_GLOBAL_SHADER(
    RayTracingGBufferRGS,
    "/Engine/Passes/RayTracing/RayTracingGBufferPipeline.hlsl",
    "RayTracingGBufferRayGeneration",
    RayGeneration);
