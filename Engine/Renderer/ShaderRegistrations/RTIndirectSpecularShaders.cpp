#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"

class RTIndirectSpecularCS final : public TGlobalShader<RTIndirectSpecularCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure | CookedShaderPackageFeatureFlags::UsesInlineRayQuery;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSpecular, IndirectSpecularTexture)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RTIndirectSpecularCS,
    RendererShaderPackages::RTIndirectSpecular,
    "Passes/Deferred/RTIndirectSpecular.hlsl",
    "main",
    Compute);
