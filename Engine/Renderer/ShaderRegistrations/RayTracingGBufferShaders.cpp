#include "PCH.h"

#include "Passes/GBuffer/RayTracing/RayTracingGBufferShaders.h"

IMPLEMENT_GLOBAL_SHADER(
    RayTracingGBufferInlineCS,
    "/Engine/Passes/GBuffer/RayTracing/RayTracingGBufferInline.hlsl",
    "RayTracingGBufferInline",
    Compute);
IMPLEMENT_GLOBAL_SHADER(
    RayTracingGBufferRGS,
    "/Engine/Passes/GBuffer/RayTracing/RayTracingGBufferPipeline.hlsl",
    "RayTracingGBufferRayGeneration",
    RayGeneration);
