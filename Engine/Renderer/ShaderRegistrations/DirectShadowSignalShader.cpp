#include "PCH.h"

#include "Passes/Lighting/Shadows/DirectShadowSignalShader.h"

IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalCS, "/Engine/Passes/Lighting/Shadows/DirectShadowSignal.hlsl",
    "main",
    Compute);
IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalRGS,
    "/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl",
    "DirectShadowSignalRayGeneration",
    RayGeneration);
IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalMiss,
    "/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl",
    "DirectShadowSignalMiss",
    Miss);
IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalClosestHit,
    "/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl",
    "DirectShadowSignalClosestHit",
    ClosestHit);
IMPLEMENT_GLOBAL_SHADER(
    DirectShadowSignalAnyHit,
    "/Engine/Passes/Lighting/Shadows/DirectShadowSignalPipeline.hlsl",
    "DirectShadowSignalAnyHit",
    AnyHit);
