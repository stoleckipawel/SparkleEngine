#include "PCH.h"

#include "Passes/RayTracing/PathTracedDirectLightingShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    PathTracedDirectLightingCS,
    RendererShaderPackages::PathTracedDirectLighting,
    "/Engine/Passes/RayTracing/PathTracedDirectLighting.hlsl",
    "main",
    Compute);
