#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Passes/RayTracing/RaytracedGBufferPass.h"
#include "Renderer/Private/RayTracing/RayTracingHitData.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"

class RaytracedGBufferCS final : public TGlobalShader<RaytracedGBufferCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure | CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferBaseColor)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferNormal)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferMaterial)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferEmissive)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferSubsurface)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferDeviceZ)
	SHADER_PARAMETER_UAV(RWTexture2D, GBufferMotionVector)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerTemporal, PerTemporalConstantBufferData, PerTemporalConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(RaytracedGBufferConstants, RaytracedGBufferUniformData, RaytracedGBufferUniformData)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitVertex, RayTracingHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RayTracingHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitInstance, RayTracingHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RayTracingHitMaterial, RayTracingHitMaterials)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(VertexSkinInfluenceData, SkinInfluences)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, JointMatrices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(JointMatrixData, PreviousJointMatrices)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RaytracedGBufferCS,
    RendererShaderPackages::RaytracedGBuffer,
    "Passes/RayTracing/RaytracedGBuffer.hlsl",
    "main",
    Compute);
