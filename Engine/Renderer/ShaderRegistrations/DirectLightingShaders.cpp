#include "PCH.h"

#include "Passes/Lighting/Direct/DirectLighting.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(DirectLightingCS, "/Engine/Passes/Lighting/Direct/DirectLighting.hlsl", "main", Compute);
