#pragma once

#include "Frame/Graph/RenderFrameGraphTargets.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowUniformData.h"
#include "RayTracing/Effects/PathTracedLighting/PathTracedLightingUniformData.h"
#include "Renderer/Public/FrameGraph/FrameGraphAccelerationStructureHandle.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterFields.h"
#include "Renderer/Public/ShaderParameters/ShaderParameterStructBuilder.h"
#include "Renderer/Public/ShaderParameters/TypedPassParameterInstance.h"
#include "Scene/Materials/MaterialTextureTableCapability.h"
#include "ShaderData/FrameUniformData.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "ShaderData/LightGpuData.h"
#include "ShaderData/SceneLightingUniformData.h"
#include "ShaderData/SkyUniformData.h"

#include <cstdint>

struct ComputePassPipelineRuntime;
struct PassCommandContext;
struct RenderPassDefinition;

struct PathTracedIndirectLightingPassParameters
{
	ShaderAccelerationStructure SceneTlas;
	ShaderUniform<FrameUniformData> Frame;
	ShaderUniform<ViewUniformData> View;
	ShaderUniform<ViewCameraUniformData> ViewCamera;
	ShaderUniform<ViewTemporalUniformData> ViewTemporal;
	ShaderUniform<SceneLightingUniformData> SceneLighting;
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
	ShaderRWTexture2D<void> IndirectDiffuse;
	ShaderRWTexture2D<void> IndirectSpecular;
	ShaderUniform<PathTracedLightingUniformData> PathTracedLightingConstants;

	static void Describe(ShaderParameterStructBuilder<PathTracedIndirectLightingPassParameters>& builder);
};

class PathTracedIndirectLightingPass final
{
public:
	static constexpr const char* PassName = "PathTracedIndirectLighting";
	static constexpr std::uint32_t ThreadGroupSizeX = 8;
	static constexpr std::uint32_t ThreadGroupSizeY = 8;
	using Parameters = PathTracedIndirectLightingPassParameters;
	using ParameterMetadata = ShaderParameterStructMetadata<Parameters>;
	using ParameterInstance = TypedPassParameterInstance<Parameters>;
	using PipelineRuntime = ComputePassPipelineRuntime;

	explicit PathTracedIndirectLightingPass(const ComputePassPipelineRuntime& runtime) noexcept;

	static const ParameterMetadata& GetParameterMetadata() noexcept;
	static const RenderPassDefinition& GetDefinition() noexcept;
	void Execute(PassCommandContext& context, ParameterInstance& parameters, std::uint32_t outputWidth, std::uint32_t outputHeight) const;

private:
	const ComputePassPipelineRuntime& m_runtime;
};
