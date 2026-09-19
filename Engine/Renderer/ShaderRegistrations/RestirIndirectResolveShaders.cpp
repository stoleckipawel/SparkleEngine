#include "PCH.h"

#include "Passes/Lighting/Restir/Indirect/RestirIndirectResolveShader.h"

IMPLEMENT_GLOBAL_SHADER(RestirIndirectResolveCS, "/Engine/Passes/Lighting/Restir/Indirect/RestirIndirectResolve.hlsl", "main", Compute);
