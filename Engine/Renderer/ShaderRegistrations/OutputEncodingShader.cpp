#include "PCH.h"

#include "Passes/Presentation/Display/OutputEncodingShader.h"

IMPLEMENT_GLOBAL_SHADER(OutputEncodingCS, "/Engine/Passes/Presentation/Display/OutputEncoding.hlsl", "main", Compute);
