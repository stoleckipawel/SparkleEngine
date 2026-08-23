#include "PCH.h"

#include "Passes/GBuffer/GBufferShaders.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(GBufferPS, "/Engine/Passes/GBuffer/GBufferPS.hlsl", "main", Pixel);
