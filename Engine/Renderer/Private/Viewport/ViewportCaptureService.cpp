#include "PCH.h"

#include "Viewport/ViewportCaptureService.h"

#include "Frame/Graph/RenderProductGraphHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

struct ViewportCaptureService::CaptureSource final
{
	const RenderProduct* Product = nullptr;
	FrameGraphResourceHandle FrameGraphResource;
	RhiResourceHandle Resource;
	ResourceState State = ResourceState::Common;
};

bool ViewportCaptureService::ResolveSource(
    const ViewportRenderProducts& products,
    FrameGraph& frameGraph,
    RenderOutputFlags output,
    CaptureSource& source,
    std::string& failureReason)
{
	source.Product = products.FindProduct(output);
	if (source.Product == nullptr || !source.Product->Handle)
	{
		failureReason = "Viewport output is not available";
		return false;
	}
	if (source.Product->Format != RenderProductFormat::ColorLdr && source.Product->Format != RenderProductFormat::ColorHdr
	    && source.Product->Format != RenderProductFormat::Float)
	{
		failureReason = "Viewport output format is not supported for readback";
		return false;
	}

	source.FrameGraphResource = ToFrameGraphResourceHandle(source.Product->Handle);
	if (!source.FrameGraphResource.IsValid())
	{
		failureReason = "Viewport output resource is not available";
		return false;
	}
	source.Resource = frameGraph.ResolveResource(FrameGraphTextureHandle{source.FrameGraphResource});
	if (!source.Resource)
	{
		failureReason = "Viewport output resource is not available";
		return false;
	}
	source.State = frameGraph.GetTrackedResourceState(source.FrameGraphResource);
	return true;
}

ViewportCaptureService::ViewportCaptureService(RenderDeviceServices& deviceServices) noexcept :
    m_deviceServices(deviceServices)
{
}

bool ViewportCaptureService::BeginCapture(
    ViewportCaptureId id,
    const ViewportCaptureRequest& request,
    const ViewportRenderProducts& products,
    FrameGraph& frameGraph,
    std::uint64_t frameId,
    std::uint64_t sceneGeneration,
    std::uint64_t providerGeneration) noexcept
{
	ViewportCaptureResult result{.FrameId = frameId, .SceneGeneration = sceneGeneration, .ProviderGeneration = providerGeneration};
	if (!id)
	{
		result.FailureReason = "Viewport capture identity is invalid";
	}
	else if (request.ExpectedFrameId != 0 && request.ExpectedFrameId != frameId)
	{
		result.FailureReason = "Viewport output belongs to a different frame identity";
	}

	CaptureSource source;
	if (!result.FailureReason.empty() || !ResolveSource(products, frameGraph, request.Output, source, result.FailureReason))
	{
		result.Status = ViewportCaptureStatus::Failed;
		m_completedCaptures.push_back(ViewportCaptureCompletion{.Id = id, .Readback = {.Result = std::move(result)}});
		return false;
	}
	result.SamplePrefix = source.Product->SamplePrefix;

	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	const RhiCaptureTicket ticket = captureService.BeginTextureReadback(
	    RhiTextureCaptureRequest{
	        .Resource = source.Resource,
	        .Width = source.Product->Extent.Width,
	        .Height = source.Product->Extent.Height,
	        .SourceFormat = frameGraph.GetTextureFormat(FrameGraphTextureHandle{source.FrameGraphResource}),
	        .SourceState = source.State,
	        .FrameId = frameId});
	if (!ticket)
	{
		result.Status = ViewportCaptureStatus::Failed;
		result.FailureReason = "The render device services could not begin viewport readback";
		m_completedCaptures.push_back(ViewportCaptureCompletion{.Id = id, .Readback = {.Result = std::move(result)}});
		return false;
	}

	m_pendingCaptures.push_back(PendingCapture{.Id = id, .Ticket = ticket, .Result = std::move(result)});
	return true;
}

void ViewportCaptureService::Poll() noexcept
{
	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	for (std::size_t index = 0; index < m_pendingCaptures.size();)
	{
		PendingCapture& pending = m_pendingCaptures[index];
		RhiCaptureReadback rhiReadback;
		if (!captureService.TryTakeTextureReadback(pending.Ticket, rhiReadback))
		{
			++index;
			continue;
		}

		m_completedCaptures.push_back(
		    ViewportCaptureCompletion{
		        .Id = pending.Id,
		        .Readback = {
		            .Result =
		                ViewportCaptureResult{
		                    .Status = rhiReadback.Result.Status == ERhiCaptureStatus::Succeeded ? ViewportCaptureStatus::Succeeded
		                                                                                        : ViewportCaptureStatus::Failed,
		                    .FrameId = rhiReadback.Result.FrameId,
		                    .SceneGeneration = pending.Result.SceneGeneration,
		                    .ProviderGeneration = pending.Result.ProviderGeneration,
		                    .SamplePrefix = pending.Result.SamplePrefix,
		                    .FailureReason = rhiReadback.Result.FailureReason},
		            .Pixels = std::move(rhiReadback.Pixels),
		            .Width = rhiReadback.Width,
		            .Height = rhiReadback.Height,
		            .RowPitch = rhiReadback.RowPitch,
		            .Format = rhiReadback.Format}});
		m_pendingCaptures.erase(m_pendingCaptures.begin() + index);
	}
}

std::vector<ViewportCaptureCompletion> ViewportCaptureService::TakeCompletedCaptures()
{
	return std::exchange(m_completedCaptures, std::vector<ViewportCaptureCompletion>{});
}
