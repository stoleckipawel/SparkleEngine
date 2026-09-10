#include "PCH.h"

#include "Passes/RayTracing/ReferencePathTracerAccumulationShader.h"

IMPLEMENT_GLOBAL_SHADER(
    ReferencePathTracerAccumulationCS,
    "/Engine/Passes/RayTracing/ReferencePathTracerAccumulation.hlsl",
    "main",
    Compute);
