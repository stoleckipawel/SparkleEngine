#include "PCH.h"

#include "Passes/RayTracing/RaytracedGBufferShader.h"

IMPLEMENT_GLOBAL_SHADER(
    RaytracedGBufferCS, "/Engine/Passes/RayTracing/RaytracedGBuffer.hlsl",
    "main",
    Compute);
