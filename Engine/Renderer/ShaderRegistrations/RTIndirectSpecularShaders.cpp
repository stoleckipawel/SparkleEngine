#include "PCH.h"

#include "RendererShaderPackages.h"
#include "Shaders/Authoring/GlobalShader.h"

#include "Resources/RenderConstantBufferData.h"
#include "Renderer/Private/RayTracing/RTIndirectSpecularHitData.h"
#include "Renderer/Private/RayTracing/RTIndirectSpecularUniformData.h"
#include "Renderer/Private/SceneData/MaterialTextureTableCapability.h"

class RTIndirectSpecularCS final : public TGlobalShader<RTIndirectSpecularCS>
{
  public:
	static constexpr CookedShaderPackageFeatureFlags kPackageFeatures =
	    CookedShaderPackageFeatureFlags::UsesAccelerationStructure | CookedShaderPackageFeatureFlags::UsesInlineRayQuery |
	    CookedShaderPackageFeatureFlags::UsesDescriptorIndexing;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV_NAMED(RWTexture2D, IndirectSpecular, IndirectSpecularTexture)
	SHADER_PARAMETER_ACCELERATION_STRUCTURE(SceneTlas)
	SHADER_PARAMETER_CBUFFER_NAMED(PerFrame, PerFrameConstantBufferData, PerFrameConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(PerView, PerViewConstantBufferData, PerViewConstantBufferData)
	SHADER_PARAMETER_CBUFFER_NAMED(RTIndirectSpecular, RTIndirectSpecularUniformData, RTIndirectSpecularUniformData)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferBaseColor)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferNormal)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferMaterial)
	SHADER_PARAMETER_TEXTURE(Texture2D, GBufferDeviceZ)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RTIndirectSpecularHitVertex, RTIndirectSpecularHitVertices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(uint32_t, RTIndirectSpecularHitIndices)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RTIndirectSpecularHitInstance, RTIndirectSpecularHitInstances)
	SHADER_PARAMETER_RDG_BUFFER_SRV(RTIndirectSpecularHitMaterial, RTIndirectSpecularHitMaterials)
	SHADER_PARAMETER_RDG_BUFFER_SRV(MeshInstanceData, MeshInstances)
	SHADER_PARAMETER_TEXTURE_ARRAY(Texture2D, MaterialTextureTable, MaterialTextureTableFixedCapacity)
	SHADER_PARAMETER_SAMPLER(SamplerState, MaterialTextureSampler)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER_IN_PACKAGE(
    RTIndirectSpecularCS,
    RendererShaderPackages::RTIndirectSpecular,
    "Passes/Deferred/RTIndirectSpecular.hlsl",
    "main",
    Compute);
