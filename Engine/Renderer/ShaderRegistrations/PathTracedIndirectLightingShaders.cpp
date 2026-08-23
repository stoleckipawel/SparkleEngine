#include "PCH.h"

#include "Passes/RayTracing/PathTracedIndirectLightingShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PathTracedIndirectLightingCS,
    RendererShaderPackages::PathTracedIndirectLighting,
    "/Engine/Passes/RayTracing/PathTracedIndirectLighting.hlsl",
    "main",
    Compute);
