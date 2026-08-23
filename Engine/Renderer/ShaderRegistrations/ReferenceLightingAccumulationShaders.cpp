#include "PCH.h"

#include "Passes/RayTracing/ReferenceLightingAccumulationShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    ReferenceLightingAccumulationCS,
    RendererShaderPackages::ReferenceLightingAccumulation,
    "/Engine/Passes/RayTracing/ReferenceLightingAccumulation.hlsl",
    "main",
    Compute);
