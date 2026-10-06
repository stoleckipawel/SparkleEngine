#include "PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"

#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerProducts.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerTransport.h"

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    ReferencePathTracerSession& session,
    RenderFrameGraphResources& resources)
{
	session.ReserveGraphResources(builder, settings.RenderExtent);
	const ReferencePathTracerGraphResources& graphResources = session.GetGraphResources();
	const ReferencePathTracerWork& work = session.GetWork();

	AddReferencePathTracerTransportPass(builder, settings.RenderExtent, resources, graphResources, work, session.m_rayTracingScene);
	AddReferencePathTracerDisplayPass(builder, settings.RenderExtent, resources, graphResources, work);

	PublishReferencePathTracerProducts(graphResources, resources);
}
