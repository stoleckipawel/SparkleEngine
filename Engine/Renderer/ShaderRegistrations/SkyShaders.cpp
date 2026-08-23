#include "PCH.h"

#include "Passes/Lighting/Sky/SkyShader.h"

IMPLEMENT_GLOBAL_SHADER(SkyCS, "/Engine/Passes/Lighting/Sky/Sky.hlsl", "main", Compute);
