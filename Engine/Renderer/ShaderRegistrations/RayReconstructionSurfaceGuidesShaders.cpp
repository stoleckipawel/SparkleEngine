#include "PCH.h"

#include "Passes/Lighting/Restir/Reconstruction/RayReconstructionSurfaceGuidesShader.h"

IMPLEMENT_GLOBAL_SHADER(RayReconstructionSurfaceGuidesCS, "/Engine/Passes/Lighting/Restir/Reconstruction/RayReconstructionSurfaceGuides.hlsl", "main", Compute);
