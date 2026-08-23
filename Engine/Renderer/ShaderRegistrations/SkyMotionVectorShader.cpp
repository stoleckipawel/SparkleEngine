#include "PCH.h"

#include "Passes/GBuffer/SkyMotionVectorShader.h"

IMPLEMENT_GLOBAL_SHADER(
    SkyMotionVectorCS, "/Engine/Passes/GBuffer/SkyMotionVector.hlsl",
    "main",
    Compute);
