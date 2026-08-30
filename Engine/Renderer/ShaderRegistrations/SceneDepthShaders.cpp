#include "PCH.h"

#include "Passes/GBuffer/SceneDepthShader.h"

IMPLEMENT_GLOBAL_SHADER(SceneDepthCS, "/Engine/Passes/GBuffer/SceneDepth.hlsl", "main", Compute);
