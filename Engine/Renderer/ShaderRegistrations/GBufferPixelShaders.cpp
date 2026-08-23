#include "PCH.h"

#include "Passes/GBuffer/GBufferShaders.h"
#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(GBufferPS, RendererShaderPackages::GBuffer, "/Engine/Passes/GBuffer/GBufferPS.hlsl", "main", Pixel);
