#include "PCH.h"

#include "Passes/GBuffer/GBufferShaders.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(GBufferVS, "/Engine/Passes/GBuffer/GBufferVS.hlsl", "main", Vertex);
