#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLighting.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    DirectLightingCS,
    RendererShaderPackages::DirectLighting,
    "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl",
    "main",
    Compute);
