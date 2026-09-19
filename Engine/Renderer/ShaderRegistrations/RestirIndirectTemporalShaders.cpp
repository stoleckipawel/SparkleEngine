#include "PCH.h"

#include "Passes/Lighting/Restir/Indirect/RestirIndirectTemporalShader.h"

IMPLEMENT_GLOBAL_SHADER(RestirIndirectTemporalCS, "/Engine/Passes/Lighting/Restir/Indirect/RestirIndirectTemporal.hlsl", "main", Compute);
