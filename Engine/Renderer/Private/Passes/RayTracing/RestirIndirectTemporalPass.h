#pragma once

#include "Frame/Targets/FrameRenderTargets.h"
#include "RayTracing/Effects/RestirLighting/RestirIndirectLightingUniformData.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/RenderConstantBufferData.h"
#include "ShaderData/RenderViewLightingData.h"
#include "ShaderData/SkyUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassExecutionContext;
struct RenderPassDefinition;

struct RestirIndirectTemporalPassParameters
{
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<PerFrameConstantBufferData> PerFrame;
	ShaderUniform<PerViewConstantBufferData> PerView;
	ShaderUniform<PerTemporalConstantBufferData> PerTemporal;
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
	ShaderRWTexture2D<void> TemporalReservoirSampleTexture;
	ShaderRWTexture2D<void> TemporalReservoirWeightTexture;
	ShaderTexture2D<void> PreviousReservoirSampleTexture;
	ShaderTexture2D<void> PreviousReservoirWeightTexture;
	ShaderTexture2D<void> PreviousReservoirSurfaceTexture;
	ShaderTexture2D<void> GBufferMotionVector;
	static void Describe(ShaderParameterStructBuilder<RestirIndirectTemporalPassParameters>& builder);
};

class RestirIndirectTemporalPass final
{
public:
	explicit RestirIndirectTemporalPass(const ComputePassPipelineRuntime& runtime) noexcept;
	static constexpr const char* PassName = "RestirIndirectTemporal";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = RestirIndirectTemporalPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;
	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassExecutionContext& context, ParameterInstance& parameters) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};
