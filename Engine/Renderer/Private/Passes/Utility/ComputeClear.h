#pragma once

#include "Renderer/Public/FrameGraph/FrameGraphTextureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStruct.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RHI/Public/Shaders/Authoring/GlobalShader.h"

#include <string_view>

class FrameGraphBuilder;

class ComputeClearCS final : public GlobalShader<ComputeClearCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(Parameters, )
	SHADER_PARAMETER_TEXTURE_UAV(RWTexture2D, Output)
	END_SHADER_PARAMETER_STRUCT()
};

void AddComputeClearPass(
    FrameGraphBuilder& builder,
    std::string_view label,
    FrameGraphTextureHandle outputTexture,
    RenderViewportExtent outputExtent);
