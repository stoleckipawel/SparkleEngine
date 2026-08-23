#include "PCH.h"
#include "Frame/FramePipeline.h"

#include "Frame/Graph/RenderProductGraphHandle.h"
#include "FrameGraph/FrameGraph.h"
#include "RHI/Public/Capture/RhiCaptureService.h"
#include "RHI/Public/Device/RenderDeviceServices.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Scene/RenderScene.h"

class FramePipelineViewportProductsImplementation final
{
public:
	static constexpr std::size_t CaptureCapacity = 3;

	struct ResolvedViewportCaptureSource final
	{
		const RenderProduct* Product = nullptr;
		FrameGraphResourceHandle FrameGraphResource;
		RhiResourceHandle Resource;
		ResourceState SourceState = ResourceState::Common;
	};

	static bool IsCapturableViewportProductFormat(RenderProductFormat format) noexcept
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

	static ViewportCapturePixelFormat ToViewportCapturePixelFormat(RhiBmpSourceFormat format) noexcept
	{
		switch (format)
		{
			case RhiBmpSourceFormat::Rgba32Float:
				return ViewportCapturePixelFormat::Rgba32Float;
			case RhiBmpSourceFormat::Rgba16Float:
				return ViewportCapturePixelFormat::Rgba16Float;
			case RhiBmpSourceFormat::Bgra8Unorm:
				return ViewportCapturePixelFormat::Bgra8Unorm;
			case RhiBmpSourceFormat::Rgba8Unorm:
			default:
				return ViewportCapturePixelFormat::Rgba8Unorm;
		}
	}

	static ViewportCaptureResult MakePendingCaptureResult(
	    const ViewportCaptureRequest& request,
	    std::uint64_t frameId,
	    std::uint64_t sceneGeneration,
	    std::uint64_t providerGeneration)
	{
		return ViewportCaptureResult{
		    .FrameId = frameId,
		    .SceneGeneration = sceneGeneration,
		    .ProviderGeneration = providerGeneration,
		    .ArtifactPath = request.OutputPath};
	}

	static bool ValidateCaptureRequest(const ViewportCaptureRequest& request, std::uint64_t frameId, ViewportCaptureResult& result)
	{
		if (request.OutputPath.empty())
		{
			result.FailureReason = "Capture output path is not set";
			return false;
		}
		if (request.ExpectedFrameId != 0 && request.ExpectedFrameId != frameId)
		{
			result.FailureReason = "Viewport output belongs to a different frame identity";
			return false;
		}
		return true;
	}

	static bool ResolveCaptureSource(
	    const ViewportRenderProducts& products,
	    FrameGraph* frameGraph,
	    RenderOutputFlags output,
	    ResolvedViewportCaptureSource& source,
	    ViewportCaptureResult& result)
	{
		source.Product = products.FindProduct(output);
		if (source.Product == nullptr || !source.Product->Handle)
		{
			result.FailureReason = "Viewport output is not available";
			return false;
		}
		if (!IsCapturableViewportProductFormat(source.Product->Format))
		{
			result.FailureReason = "Viewport output format is not supported for BMP capture";
			return false;
		}

		source.FrameGraphResource = ToFrameGraphResourceHandle(source.Product->Handle);
		if (!source.FrameGraphResource.IsValid() || frameGraph == nullptr)
		{
			result.FailureReason = "Viewport output resource is not available";
			return false;
		}
		source.Resource = frameGraph->ResolveResource(FrameGraphTextureHandle{source.FrameGraphResource});
		if (!source.Resource)
		{
			result.FailureReason = "Viewport output resource is not available";
			return false;
		}
		source.SourceState = frameGraph->GetTrackedResourceState(source.FrameGraphResource);
		return true;
	}
};

bool FramePipeline::BeginViewportCapture(ViewportCaptureId id, const ViewportCaptureRequest& request) noexcept
{
	const std::uint64_t frameId = m_frameId;
	const std::uint64_t sceneGeneration = m_renderScene.GetSceneGeneration();
	const std::uint64_t providerGeneration = m_imageProviders.GetGeneration();
	ViewportCaptureResult result =
	    FramePipelineViewportProductsImplementation::MakePendingCaptureResult(request, frameId, sceneGeneration, providerGeneration);
	if (!id || m_pendingViewportCaptures.size() >= FramePipelineViewportProductsImplementation::CaptureCapacity
	    || !FramePipelineViewportProductsImplementation::ValidateCaptureRequest(request, frameId, result))
	{
		result.Status = ViewportCaptureStatus::Failed;
		m_completedViewportCaptures.push_back(ViewportCaptureReadback{.Id = id, .Result = std::move(result)});
		return false;
	}
	FramePipelineViewportProductsImplementation::ResolvedViewportCaptureSource source;
	if (!FramePipelineViewportProductsImplementation::ResolveCaptureSource(
	        m_viewportRenderProducts,
	        m_frameGraph.get(),
	        request.Output,
	        source,
	        result))
	{
		result.Status = ViewportCaptureStatus::Failed;
		m_completedViewportCaptures.push_back(ViewportCaptureReadback{.Id = id, .Result = std::move(result)});
		return false;
	}

	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	const RhiCaptureTicket ticket = captureService.BeginTextureReadback(
	    RhiTextureCaptureRequest{
	        .Resource = source.Resource,
	        .Width = source.Product->Extent.Width,
	        .Height = source.Product->Extent.Height,
	        .SourceFormat = m_frameGraph->GetTextureFormat(FrameGraphTextureHandle{source.FrameGraphResource}),
	        .SourceState = source.SourceState,
	        .OutputPath = request.OutputPath,
	        .FrameId = frameId,
	        .ViewMode = request.ViewMode,
	        .ViewModeName = request.ViewModeName,
	        .DebugName = request.DebugName});
	if (!ticket)
	{
		result.Status = ViewportCaptureStatus::Failed;
		result.FailureReason = "The render device services could not begin viewport readback";
		m_completedViewportCaptures.push_back(ViewportCaptureReadback{.Id = id, .Result = std::move(result)});
		return false;
	}
	m_pendingViewportCaptures.push_back(
	    std::make_unique<PendingViewportCapture>(PendingViewportCapture{
	        .Id = id,
	        .Ticket = ticket,
	        .SceneGeneration = sceneGeneration,
	        .ProviderGeneration = providerGeneration}));
	return true;
}

void FramePipeline::PollViewportCaptures() noexcept
{
	RhiCaptureService& captureService = m_deviceServices.GetRenderHardwareInterface().GetCaptureService();
	for (std::size_t index = 0; index < m_pendingViewportCaptures.size();)
	{
		const std::unique_ptr<PendingViewportCapture>& pending = m_pendingViewportCaptures[index];
		RhiCaptureReadback rhiReadback;
		if (!captureService.TryTakeTextureReadback(pending->Ticket, rhiReadback))
		{
			++index;
			continue;
		}

		if (m_completedViewportCaptures.size() >= FramePipelineViewportProductsImplementation::CaptureCapacity)
		{
			m_completedViewportCaptures.erase(m_completedViewportCaptures.begin());
		}
		m_completedViewportCaptures.push_back(
		    ViewportCaptureReadback{
		        .Id = pending->Id,
		        .Result =
		            ViewportCaptureResult{
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
		        .Format = FramePipelineViewportProductsImplementation::ToViewportCapturePixelFormat(rhiReadback.Format)});
		m_pendingViewportCaptures.erase(m_pendingViewportCaptures.begin() + index);
	}
}

std::vector<ViewportCaptureReadback> FramePipeline::TakeCompletedViewportCaptures()
{
	return std::exchange(m_completedViewportCaptures, std::vector<ViewportCaptureReadback>{});
}
