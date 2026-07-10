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

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(GBufferVS, RendererShaderPackages::GBuffer, "Passes/Deferred/GBufferVS.hlsl", "main", Vertex);
