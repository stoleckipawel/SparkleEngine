#include "PCH.h"

#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(GBufferPS, "/Engine/Passes/GBuffer/Raster/GBufferPS.hlsl", "main", Pixel);
