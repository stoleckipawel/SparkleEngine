#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Commands/RenderCommandContext.h"
#include "Frame/Core/RenderProductHandleUtils.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Commands/RenderCommandList.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Presentation/RhiPresentationService.h"

namespace
{
	bool IsCapturableViewportProductFormat(RenderProductFormat format) noexcept
	{
		switch (format)
		{
			case RenderProductFormat::ColorLdr:
			case RenderProductFormat::ColorHdr:
			case RenderProductFormat::Float:
				return true;
			default:
				return false;
		}
	}

	ViewportCaptureResult ToViewportCaptureResult(const RhiCaptureResult& rhiResult) noexcept
	{
		ViewportCaptureResult result{};
		result.ArtifactPath = rhiResult.ArtifactPath;
		result.FailureReason = rhiResult.FailureReason;
		switch (rhiResult.Status)
		{
			case ERhiCaptureStatus::Succeeded:
				result.Status = ViewportCaptureStatus::Succeeded;
				break;
			case ERhiCaptureStatus::Unsupported:
				result.Status = ViewportCaptureStatus::Unavailable;
				break;
			case ERhiCaptureStatus::Failed:
			default:
				result.Status = ViewportCaptureStatus::Failed;
				break;
		}
		return result;
	}
}

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

ViewportCaptureResult FramePipeline::CaptureViewportProductToBmp(const ViewportCaptureRequest& request) noexcept
{
	ViewportCaptureResult result{};
	result.ArtifactPath = request.OutputPath;
	if (request.OutputPath.empty())
	{
		result.FailureReason = "Capture output path is not set";
		return result;
	}

	const RenderProduct* product = m_viewportRenderProducts.FindProduct(request.Output);
	if (product == nullptr || !product->Handle)
	{
		result.FailureReason = "Viewport output is not available";
		return result;
	}

	if (!IsCapturableViewportProductFormat(product->Format))
	{
		result.FailureReason = "Viewport output format is not supported for BMP capture";
		return result;
	}

	const FrameGraphResourceHandle resourceHandle = ResolveRenderProductResourceHandle(product->Handle);
	const RhiResourceHandle resource = resourceHandle.IsValid() && m_frameGraph != nullptr ?
	                                          m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle}) :
	                                          RhiResourceHandle{};
	if (!resource)
	{
		result.FailureReason = "Viewport output resource is not available";
		return result;
	}

	const ResourceState sourceState = m_frameGraph->GetTrackedResourceState(resourceHandle);
	return ToViewportCaptureResult(m_systems->GetRenderHardwareInterface().GetCaptureService().CaptureTextureToBmp(
	    RhiTextureCaptureRequest{
	        .Resource = resource,
	        .Width = product->Extent.Width,
	        .Height = product->Extent.Height,
	        .SourceState = sourceState,
	        .OutputPath = request.OutputPath,
	        .FrameIndex = request.FrameIndex,
	        .ViewMode = request.ViewMode,
	        .ViewModeName = request.ViewModeName,
	        .DebugName = request.DebugName}));
}

FrameGraphResourceHandle FramePipeline::ResolveRenderProductResourceHandle(RenderProductHandle handle) const noexcept
{
	return ToFrameGraphResourceHandle(handle);
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

	const RhiResourceHandle resource = m_frameGraph->ResolveResource(FrameGraphTextureHandle{resourceHandle});
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
