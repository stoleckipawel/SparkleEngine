#include "PCH.h"

#include "Passes/GBuffer/Raster/GBufferShaders.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(GBufferVS, "/Engine/Passes/GBuffer/Raster/GBufferVS.hlsl", "main", Vertex);
