#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLightReservoirTemporalShader.h"

IMPLEMENT_GLOBAL_SHADER(
    DirectLightReservoirTemporalCS, "/Engine/Passes/Lighting/Direct/DirectLightReservoirTemporal.hlsl",
    "main",
    Compute);
