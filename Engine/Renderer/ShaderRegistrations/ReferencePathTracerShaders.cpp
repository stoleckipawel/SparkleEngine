#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerInlineCS,
    "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerInline.hlsl",
    "ReferencePathTracerInline",
    Compute);
IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerRGS,
    "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerPipeline.hlsl",
    "ReferencePathTracerRayGeneration",
    RayGeneration);
IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerDisplayCS,
    "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.hlsl",
    "main",
    Compute);
