#include "PCH.h"

#include "Passes/Core/ComputeClear.h"
#include "Shaders/Authoring/GlobalShader.h"

IMPLEMENT_GLOBAL_SHADER(ComputeClearCS, "/Engine/Passes/Compute/ComputeClear.hlsl", "main", Compute);
