#include "PCH.h"

#include "Passes/Presentation/OutputEncodingShader.h"

IMPLEMENT_GLOBAL_SHADER(OutputEncodingCS, "/Engine/Passes/Presentation/OutputEncoding.hlsl", "main", Compute);
