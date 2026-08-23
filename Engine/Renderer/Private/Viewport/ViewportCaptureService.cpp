#include "PCH.h"

#include "Viewport/ViewportCaptureService.h"

#include "Frame/Graph/RenderProductGraphHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

class ViewportCaptureSourceResolver final
{
public:
	struct Source final
	{
		const RenderProduct* Product = nullptr;
		FrameGraphResourceHandle FrameGraphResource;
		RhiResourceHandle Resource;
		ResourceState State = ResourceState::Common;
	};

	static bool IsCapturable(RenderProductFormat format) noexcept
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

	static bool Resolve(
	    const ViewportRenderProducts& products,
	    FrameGraph* frameGraph,
	    RenderOutputFlags output,
	    Source& source,
	    std::string& failureReason)
	{
		source.Product = products.FindProduct(output);
		if (source.Product == nullptr || !source.Product->Handle)
		{
			failureReason = "Viewport output is not available";
			return false;
		}
		if (!IsCapturable(source.Product->Format))
		{
			failureReason = "Viewport output format is not supported for BMP capture";
			return false;
		}

		source.FrameGraphResource = ToFrameGraphResourceHandle(source.Product->Handle);
		if (!source.FrameGraphResource.IsValid() || frameGraph == nullptr)
		{
			failureReason = "Viewport output resource is not available";
			return false;
		}
		source.Resource = frameGraph->ResolveResource(FrameGraphTextureHandle{source.FrameGraphResource});
		if (!source.Resource)
		{
			failureReason = "Viewport output resource is not available";
			return false;
		}
		source.State = frameGraph->GetTrackedResourceState(source.FrameGraphResource);
		return true;
	}
};

ViewportCaptureService::ViewportCaptureService(RenderDeviceServices& deviceServices) noexcept :
    m_deviceServices(deviceServices)
{
}

bool ViewportCaptureService::BeginCapture(
    ViewportCaptureId id,
    const ViewportCaptureRequest& request,
    const ViewportRenderProducts& products,
    FrameGraph* frameGraph,
    std::uint64_t frameId,
    std::uint64_t sceneGeneration,
    std::uint64_t providerGeneration) noexcept
{
	ViewportCaptureResult result{
	    .FrameId = frameId,
	    .SceneGeneration = sceneGeneration,
	    .ProviderGeneration = providerGeneration,
	    .ArtifactPath = request.OutputPath};
	if (!id || m_pendingCaptures.size() >= CaptureCapacity)
	{
		result.FailureReason = "Viewport capture capacity is exhausted";
	}
	else if (request.OutputPath.empty())
	{
		result.FailureReason = "Capture output path is not set";
	}
	else if (request.ExpectedFrameId != 0 && request.ExpectedFrameId != frameId)
	{
		result.FailureReason = "Viewport output belongs to a different frame identity";
	}

	ViewportCaptureSourceResolver::Source source;
	if (!result.FailureReason.empty()
	    || !ViewportCaptureSourceResolver::Resolve(products, frameGraph, request.Output, source, result.FailureReason))
	{
		result.Status = ViewportCaptureStatus::Failed;
		m_completedCaptures.push_back(ViewportCaptureReadback{.Id = id, .Result = std::move(result)});
		return false;
	}

	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	const RhiCaptureTicket ticket = captureService.BeginTextureReadback(
	    RhiTextureCaptureRequest{
	        .Resource = source.Resource,
	        .Width = source.Product->Extent.Width,
	        .Height = source.Product->Extent.Height,
	        .SourceFormat = frameGraph->GetTextureFormat(FrameGraphTextureHandle{source.FrameGraphResource}),
	        .SourceState = source.State,
	        .OutputPath = request.OutputPath,
	        .FrameId = frameId,
	        .ViewMode = request.ViewMode,
	        .ViewModeName = request.ViewModeName,
	        .DebugName = request.DebugName});
	if (!ticket)
	{
		result.Status = ViewportCaptureStatus::Failed;
		result.FailureReason = "The render device services could not begin viewport readback";
		m_completedCaptures.push_back(ViewportCaptureReadback{.Id = id, .Result = std::move(result)});
		return false;
	}

	m_pendingCaptures.push_back(
	    std::make_unique<PendingCapture>(PendingCapture{
	        .Id = id,
	        .Ticket = ticket,
	        .SceneGeneration = sceneGeneration,
	        .ProviderGeneration = providerGeneration}));
	return true;
}

void ViewportCaptureService::Poll() noexcept
{
	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	for (std::size_t index = 0; index < m_pendingCaptures.size();)
	{
		const std::unique_ptr<PendingCapture>& pending = m_pendingCaptures[index];
		RhiCaptureReadback rhiReadback;
		if (!captureService.TryTakeTextureReadback(pending->Ticket, rhiReadback))
		{
			++index;
			continue;
		}

		if (m_completedCaptures.size() >= CaptureCapacity)
		{
			m_completedCaptures.erase(m_completedCaptures.begin());
		}
		m_completedCaptures.push_back(
		    ViewportCaptureReadback{
		        .Id = pending->Id,
		        .Result = ViewportCaptureResult{
		            .Status = rhiReadback.Result.Status == ERhiCaptureStatus::Succeeded ? ViewportCaptureStatus::Succeeded
		                                                                                 : ViewportCaptureStatus::Failed,
		            .FrameId = rhiReadback.Result.FrameId,
		            .SceneGeneration = pending->SceneGeneration,
		            .ProviderGeneration = pending->ProviderGeneration,
		            .ArtifactPath = rhiReadback.Result.ArtifactPath,
		            .FailureReason = rhiReadback.Result.FailureReason},
		        .Pixels = std::move(rhiReadback.Pixels),
		        .Width = rhiReadback.Width,
		        .Height = rhiReadback.Height,
		        .RowPitch = rhiReadback.RowPitch,
		        .Format = rhiReadback.Format});
		m_pendingCaptures.erase(m_pendingCaptures.begin() + index);
	}
}

std::vector<ViewportCaptureReadback> ViewportCaptureService::TakeCompletedCaptures()
{
	return std::exchange(m_completedCaptures, std::vector<ViewportCaptureReadback>{});
}
