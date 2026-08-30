#include "PCH.h"

#include "Passes/Debug/VisualizeBuffersShader.h"

IMPLEMENT_GLOBAL_SHADER(VisualizeBuffersCS, "/Engine/Passes/Debug/VisualizeBuffers.hlsl", "main", Compute);
