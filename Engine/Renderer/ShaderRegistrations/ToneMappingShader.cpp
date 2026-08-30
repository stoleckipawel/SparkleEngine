#include "PCH.h"

#include "Passes/Presentation/ToneMappingShader.h"

IMPLEMENT_GLOBAL_SHADER(ToneMappingCS, "/Engine/Passes/Presentation/ToneMapping.hlsl", "main", Compute);
