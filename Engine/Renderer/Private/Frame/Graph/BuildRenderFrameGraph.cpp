#include "../../PCH.h"
#include "Frame/FramePipeline.h"
#include "Frame/RenderFrame.h"

#include "Frame/Graph/RenderFrameGraphResources.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "Passes/Presentation/PresentationPasses.h"
#include "Passes/Scene/RayTracingScenePass.h"
#include "Scene/RenderScene.h"
#include "Scene/RenderSceneFrameGraphBindings.h"
#include "Frame/Graph/ViewportFrameProductExports.h"

void FramePipeline::PrepareFrameGraph(const RenderFrame& frame)
{
	m_frameGraph->BeginFrame();
	FrameGraphBuilder builder(*m_frameGraph, m_renderPassRuntimeCache);
	m_frameResources = BuildRenderFrameGraph(builder, frame);
	BindRenderSceneFrameGraphResources(*m_frameGraph, m_frameResources, frame.PreparedScene, frame.RayTracingBindings);
}

RenderFrameGraphResources FramePipeline::BuildRenderFrameGraph(FrameGraphBuilder& builder, const RenderFrame& frame)
{
	RenderFrameGraphResources resources = CreateRenderFrameGraphResources(builder, m_frameGraphSettings);
	AddRayTracingScenePass(builder, frame, m_renderScene->GetRayTracingScene(), resources);
	AddSceneRenderingPasses(builder, frame, resources);
	AddPresentationPasses(builder, frame, m_frameGraphSettings, resources);
	ExportViewportFrameProducts(builder, m_frameGraphSettings, resources);
	return resources;
}
