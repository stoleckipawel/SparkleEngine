#include "PCH.h"

#include "Passes/Lighting/Sky/SkyShader.h"
#include "RendererShaderPackages.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(SkyCS, RendererShaderPackages::Sky, "/Engine/Passes/Lighting/Sky/Sky.hlsl", "main", Compute);
