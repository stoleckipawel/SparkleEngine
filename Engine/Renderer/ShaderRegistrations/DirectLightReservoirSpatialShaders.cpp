#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLightReservoirSpatialShader.h"

IMPLEMENT_GLOBAL_SHADER(
    DirectLightReservoirSpatialCS, "/Engine/Passes/Lighting/Direct/DirectLightReservoirSpatial.hlsl",
    "main",
    Compute);
