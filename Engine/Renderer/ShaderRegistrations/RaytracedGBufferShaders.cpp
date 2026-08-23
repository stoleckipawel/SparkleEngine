#include "PCH.h"

#include "Passes/RayTracing/RaytracedGBufferShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RaytracedGBufferCS,
    RendererShaderPackages::RaytracedGBuffer,
    "/Engine/Passes/RayTracing/RaytracedGBuffer.hlsl",
    "main",
    Compute);
