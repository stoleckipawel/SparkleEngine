#include "PCH.h"

#include "Passes/Presentation/Display/ToneMappingShader.h"

IMPLEMENT_GLOBAL_SHADER(ToneMappingCS, "/Engine/Passes/Presentation/Display/ToneMapping.hlsl", "main", Compute);
