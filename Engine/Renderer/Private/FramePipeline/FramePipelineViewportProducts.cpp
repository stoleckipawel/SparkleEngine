#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"

ViewportPresentationProduct FramePipeline::BeginViewportPresentation(RenderOutputFlags output) noexcept
{
	const RenderProduct* product = m_viewportRenderProducts.FindProduct(output);
	if (product == nullptr || !product->Handle)
	{
		return ViewportPresentationProduct{
		    .Output = output,
		    .FailureReason = "Viewport output is not available"};
	}

	if (m_frameGraph == nullptr)
	{
		return ViewportPresentationProduct{
		    .Output = output,
		    .Product = *product,
		    .FailureReason = "Frame graph is not available"};
	}

	TransitionRenderProduct(product->Handle, ResourceState::ShaderResource);

	const FrameGraphResourceHandle resourceHandle = ResolveRenderProductResourceHandle(product->Handle);
	const std::uint64_t textureId = m_systems->GetRenderHardwareInterface().GetPresentationService().ResolveImGuiTextureId(
	    m_frameGraph->ResolveShaderResourceView(FrameGraphTextureHandle{resourceHandle}));
	if (textureId == 0)
	{
		return ViewportPresentationProduct{
		    .Output = output,
		    .Product = *product,
		    .FailureReason = "RHI presentation service did not resolve a texture id"};
	}

	return ViewportPresentationProduct{
	    .Output = output,
	    .Product = *product,
	    .TextureId = textureId,
	    .Status = ViewportPresentationStatus::Ready};
}

void FramePipeline::EndViewportPresentation(RenderOutputFlags output) noexcept
{
	const RenderProduct* product = m_viewportRenderProducts.FindProduct(output);
	if (product == nullptr || !product->Handle)
	{
		return;
	}

	TransitionRenderProduct(product->Handle, ResourceState::Common);
}

RhiCaptureResult FramePipeline::CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept
{
	RhiCaptureResult result{};
	result.BackendApi = m_systems->GetRenderHardwareInterface().GetCapabilities().BackendApi;
	result.FrameIndex = request.FrameIndex;
	result.ViewMode = request.ViewMode;
	result.ViewModeName = request.ViewModeName;
	result.ArtifactPath = request.OutputPath;

	const RenderProduct* product = m_viewportRenderProducts.FindProduct(request.Output);
	if (product == nullptr || !product->Handle)
	{
		result.FailureReason = "Viewport output is not available";
		return result;
	}

	const NativeResourceHandle resource = ResolveRenderProductResource(product->Handle);
	if (!resource)
	{
		result.FailureReason = "Viewport output resource is not available";
		return result;
	}

	return m_systems->GetRenderHardwareInterface().GetCaptureService().CaptureTextureToBmp(
	    RhiTextureCaptureRequest{
	        .Resource = resource,
	        .Width = product->Extent.Width,
	        .Height = product->Extent.Height,
	        .OutputPath = request.OutputPath,
	        .FrameIndex = request.FrameIndex,
	        .ViewMode = request.ViewMode,
	        .ViewModeName = request.ViewModeName,
	        .DebugName = request.DebugName});
}

FrameGraphResourceHandle FramePipeline::ResolveRenderProductResourceHandle(RenderProductHandle handle) const noexcept
{
	return ToFrameGraphResourceHandle(handle);
}

NativeResourceHandle FramePipeline::ResolveRenderProductResource(RenderProductHandle handle) const noexcept
{
	if (!handle || !m_frameGraph)
	{
		return NativeResourceHandle{};
	}

	const FrameGraphResourceHandle resourceHandle = ResolveRenderProductResourceHandle(handle);
	if (!resourceHandle.IsValid())
	{
		return NativeResourceHandle{};
	}

	return m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
}

void FramePipeline::TransitionRenderProduct(RenderProductHandle handle, ResourceState after) noexcept
{
	if (!handle || !m_frameGraph)
	{
		return;
	}

	const FrameGraphResourceHandle resourceHandle = ResolveRenderProductResourceHandle(handle);
	if (!resourceHandle.IsValid())
	{
		return;
	}

	const NativeResourceHandle resource = m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
	if (!resource)
	{
		return;
	}

	const ResourceState trackedBefore = m_frameGraph->GetTrackedResourceState(resourceHandle);
	if (trackedBefore == after)
	{
		return;
	}

	RenderHardwareInterface& renderHardware = m_systems->GetRenderHardwareInterface();
	RenderCommandList& commandList = m_systems->GetBackend().GetGraphicsCommandList(renderHardware.GetCurrentFrameIndex());
	commandList.TransitionResource(resource, trackedBefore, after);
	m_frameGraph->UpdateTrackedResourceState(resourceHandle, after);
}
