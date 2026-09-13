#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerShader.h"

IMPLEMENT_GLOBAL_SHADER(ReferencePathTracerCS, "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracer.hlsl", "main", Compute);
IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerDisplayCS,
    "/Engine/Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.hlsl",
    "main",
    Compute);
