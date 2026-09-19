#include "PCH.h"

#include "Passes/Lighting/Restir/Indirect/RestirIndirectSpatialShader.h"

IMPLEMENT_GLOBAL_SHADER(RestirIndirectSpatialCS, "/Engine/Passes/Lighting/Restir/Indirect/RestirIndirectSpatial.hlsl", "main", Compute);
