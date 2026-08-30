#include "PCH.h"

#include "Passes/RayTracing/RestirIndirectSpatialShader.h"

IMPLEMENT_GLOBAL_SHADER(RestirIndirectSpatialCS, "/Engine/Passes/RayTracing/RestirIndirectSpatial.hlsl", "main", Compute);
