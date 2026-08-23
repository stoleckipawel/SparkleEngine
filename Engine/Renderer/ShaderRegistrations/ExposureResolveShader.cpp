#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Renderer/Private/ShaderData/ExposureUniformData.h"

class ExposureCS final : public TGlobalShader<ExposureCS>
{
public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, ExposureTexture)
	SHADER_PARAMETER_UAV(RWTexture2D, ExposureHistoryTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, PreviousExposureTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, LuminanceMoments)
	SHADER_PARAMETER_CBUFFER_NAMED(ExposureConstants, ExposureUniformData, ExposureUniformData)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(ExposureCS, RendererShaderPackages::Exposure, "Passes/PostProcessing/Exposure.hlsl", "main", Compute);
