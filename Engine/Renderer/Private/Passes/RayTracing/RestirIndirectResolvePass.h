#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/RenderViewLightingData.h"
#include "ShaderData/SkyUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct RestirIndirectResolvePassParameters
{
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<FrameUniformData> Frame;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<ViewLightingData> ViewLighting;
	ShaderUniform<RayTracedShadowUniformData> RayTracedShadows;
	ShaderUniform<SkyUniformData> Sky;
	ShaderTexture2D<void> GBufferBaseColor;
	ShaderTexture2D<void> GBufferNormal;
	ShaderTexture2D<void> GBufferMaterial;
	ShaderTexture2D<void> SceneDepth;
	ShaderTexture2D<void> SkyTexture;
	ShaderSamplerSet SamplerLinearClamp;
	ShaderBuffer<void> RayTracingHitVertices;
	ShaderBuffer<void> MorphTargetDeltas;
	ShaderBuffer<void> RayTracingHitIndices;
	ShaderBuffer<void> RayTracingHitInstances;
	ShaderBuffer<void> RayTracingHitMaterials;
	ShaderBuffer<void> MeshInstances;
	ShaderBuffer<void> SkinInfluences;
	ShaderBuffer<void> JointMatrices;
	ShaderBuffer<void> MorphWeights;
	ShaderBuffer<void> DirectionalLights;
	ShaderBuffer<void> PointLights;
	ShaderBuffer<void> SpotLights;
	ShaderBuffer<void> RectLights;
	ShaderTexture2DTableSRV<MaterialTextureTableFixedCapacity> MaterialTextureTable;
	ShaderSamplerSet MaterialTextureSampler;
	ShaderUniform<RestirIndirectLightingUniformData> RestirIndirectConstants;
	ShaderTexture2D<void> CurrentReservoirSampleTexture;
	ShaderTexture2D<void> CurrentReservoirWeightTexture;
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderRWTexture2D<void> RayReconstructionDiffuseAlbedo;
	ShaderRWTexture2D<void> RayReconstructionSpecularAlbedo;
	ShaderRWTexture2D<void> RayReconstructionRoughness;
	ShaderRWTexture2D<void> RayReconstructionSpecularHitDistance;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectResolvePassParameters>& builder);
};

class RestirIndirectResolvePass final
{
public:
	explicit RestirIndirectResolvePass(const ComputePassPipelineRuntime& runtime) noexcept;
	static constexpr const char* PassName = "RestirIndirectResolve";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RestirIndirectResolvePassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};
