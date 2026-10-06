#include "PCH.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerPasses.h"

#include "Frame/RenderFrame.h"
#include "Frame/Graph/RenderFrameGraphSettings.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerDisplay.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerProducts.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerResources.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerSession.h"
#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerTransport.h"

void AddReferencePathTracerPasses(
    FrameGraphBuilder& builder,
    const RenderFrame& frame,
    const RenderFrameGraphSettings& settings,
    ReferencePathTracerSession& session,
    RenderFrameGraphResources& resources)
{
	session.ReserveGraphResources(builder, settings.RenderExtent);
	if (!session.BindResources(builder))
	{
		return;
	}
	const ReferencePathTracerGraphResources& graphResources = session.GetGraphResources();
	const ReferencePathTracerWork& work = session.GetWork();

	AddReferencePathTracerTransportPass(builder, frame, settings.RenderExtent, resources, graphResources, work, session.m_rayTracingScene);
	AddReferencePathTracerDisplayPass(builder, settings.RenderExtent, resources, graphResources, work);

	PublishReferencePathTracerProducts(graphResources, resources);
}
