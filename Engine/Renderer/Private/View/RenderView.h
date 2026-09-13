#pragma once

#include "Core/Public/Math/Frustum.h"
#include "GameFramework/Public/Rendering/RenderViewCameraData.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"
#include "RayTracing/Acceleration/RayTracingPtlasPartitionPlanner.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"
#include "ShaderData/ViewUniformData.h"
#include "ShaderData/ViewCameraUniformData.h"
#include "ShaderData/ViewTemporalUniformData.h"
#include "View/MeshInstanceBatch.h"
#include "View/RenderViewWorkloadSummary.h"
#include "View/ViewportDisplaySettings.h"

#include <cstdint>
#include <vector>

struct RenderView final
{
	std::uint64_t viewportId = 0u;
	RenderViewSelectionToken selection = {};
	RenderViewKind kind = RenderViewKind::Game;
	RenderViewportExtent renderExtent = {};
	RenderViewportExtent outputExtent = {};
	RenderViewCameraData camera = {};
	Frustum frustum = {};
	RhiViewport viewport = {};
	RhiRect scissorRect = {};
	ResolvedViewportDisplaySettings displaySettings = {};
	ViewUniformData uniform = {};
	ViewCameraUniformData cameraUniform = {};
	ViewTemporalUniformData temporalUniform = {};
	std::vector<std::uint32_t> rasterPrimitiveIndices;
	std::vector<MeshInstanceBatch> meshInstanceBatches;
	RayTracingPtlasPartitionPlan rayTracingPlan = {};
	RenderViewWorkloadSummary workload = {};

	void ResetForReuse() noexcept;
};
