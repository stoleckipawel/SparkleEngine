#include "PCH.h"

#include "Passes/GBuffer/SkyMotionVectorShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    SkyMotionVectorCS,
    RendererShaderPackages::SkyMotionVector,
    "/Engine/Passes/GBuffer/SkyMotionVector.hlsl",
    "main",
    Compute);
