#include "PCH.h"

#include "Viewport/ViewportRenderProductPublication.h"

#include "Frame/Graph/RenderProductGraphHandle.h"

void PublishViewportRenderProducts(
    ViewportRenderProducts& products,
    const ViewportRenderRequest& request,
    const ViewportFrameProducts& frameProducts,
    RenderViewportExtent renderExtent,
    RenderViewportExtent outputExtent) noexcept
{
	products.Clear();
	products.SetGeneration(request.Generation);
	products.SetProduct(
	    RenderOutputFlags::SceneColor,
	    RenderProduct{
	        .Handle = ToRenderProductHandle(frameProducts.FinalSceneColor),
	        .Extent = outputExtent,
	        .Format = RenderProductFormat::ColorLdr});

	if (frameProducts.SceneDepth.IsValid() && HasAnyRenderOutputFlags(request.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		products.SetProduct(
		    RenderOutputFlags::SceneDepth,
		    RenderProduct{
		        .Handle = ToRenderProductHandle(frameProducts.SceneDepth),
		        .Extent = renderExtent,
		        .Format = RenderProductFormat::Float});
	}

	if (frameProducts.Normals.IsValid() && HasAnyRenderOutputFlags(request.RequestedOutputs, RenderOutputFlags::Normals))
	{
		products.SetProduct(
		    RenderOutputFlags::Normals,
		    RenderProduct{
		        .Handle = ToRenderProductHandle(frameProducts.Normals),
		        .Extent = renderExtent,
		        .Format = RenderProductFormat::ColorHdr});
	}
}
