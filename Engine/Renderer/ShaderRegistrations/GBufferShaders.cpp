#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "ShaderData/RenderConstantBufferData.h"

class GBufferVS final : public TGlobalShader<GBufferVS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(MeshInstanceDraw, MeshInstanceDrawConstantBufferData, MeshInstanceDrawConstantBufferData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, PreviousJointMatrices)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    GBufferVS,
    RendererShaderPackages::GBuffer,
    "Passes/Deferred/GBufferVS.hlsl",
    "main",
    Vertex);

class GBufferPS final : public TGlobalShader<GBufferPS>
{
  public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerObjectPS, PerObjectPSConstantBufferData, PerObjectPSConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureRoughness)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureMetallic)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureOcclusion)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureEmissive)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureSubsurfaceColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, TextureSubsurfaceStrength)
	SHADER_PARAMETER_SHARED_SAMPLER(SamplerAniso16xWrap)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    GBufferPS,
    RendererShaderPackages::GBuffer,
    "Passes/Deferred/GBufferPS.hlsl",
    "main",
    Pixel);
