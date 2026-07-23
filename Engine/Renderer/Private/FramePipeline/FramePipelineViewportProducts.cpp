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
#include "RHI/Public/UI/RhiImGuiRenderer.h"
#include "SceneData/Input/RenderInputConsumer.h"

class FramePipelineViewportProductsImplementation final
{
  public:
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

	static ViewportCaptureResult ToViewportCaptureResult(
	    const RhiCaptureResult& rhiResult,
	    const RenderFrameMetadata& metadata) noexcept
	{
		ViewportCaptureResult result{};
		result.FrameId = rhiResult.FrameId;
		result.FrameGeneration = metadata.FrameGeneration;
		result.ProviderGeneration = metadata.ProviderGeneration;
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

	static ViewportCaptureResult MakePendingCaptureResult(
	    const ViewportCaptureRequest& request,
	    const RenderFrameMetadata& metadata)
	{
		return ViewportCaptureResult{
		    .FrameId = metadata.FrameId,
		    .FrameGeneration = metadata.FrameGeneration,
		    .ProviderGeneration = metadata.ProviderGeneration,
		    .ArtifactPath = request.OutputPath};
	}

	static bool ValidateCaptureRequest(
	    const ViewportCaptureRequest& request,
	    const RenderFrameMetadata& metadata,
	    ViewportCaptureResult& result)
	{
		if (request.OutputPath.empty())
		{
			result.FailureReason = "Capture output path is not set";
			return false;
		}
		if (request.ExpectedFrameId != 0 && request.ExpectedFrameId != metadata.FrameId)
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
	const std::uint64_t textureId = m_systems->GetBackend().GetImGuiRenderer().ResolveTextureId(
	    m_frameGraph->ResolveShaderResourceView(FrameGraphTextureHandle{resourceHandle}));
	if (textureId == 0)
	{
		return ViewportPresentationProduct{
		    .Output = output,
		    .Product = *product,
		    .FailureReason = "RHI ImGui renderer did not resolve a texture id"};
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
	const RenderFrameMetadata& metadata = m_renderInputConsumer->GetDynamicData().Metadata;
	ViewportCaptureResult result = FramePipelineViewportProductsImplementation::MakePendingCaptureResult(request, metadata);
	if (!FramePipelineViewportProductsImplementation::ValidateCaptureRequest(request, metadata, result)) return result;
	FramePipelineViewportProductsImplementation::ResolvedViewportCaptureSource source;
	if (!FramePipelineViewportProductsImplementation::ResolveCaptureSource(m_viewportRenderProducts, m_frameGraph.get(), request.Output, source, result))
		return result;

	return FramePipelineViewportProductsImplementation::ToViewportCaptureResult(m_systems->GetRenderHardwareInterface().GetCaptureService().CaptureTextureToBmp(
	    RhiTextureCaptureRequest{
	        .Resource = source.Resource,
	        .Width = source.Product->Extent.Width,
	        .Height = source.Product->Extent.Height,
	        .SourceState = source.SourceState,
	        .OutputPath = request.OutputPath,
	        .FrameId = metadata.FrameId,
	        .ViewMode = request.ViewMode,
	        .ViewModeName = request.ViewModeName,
	        .DebugName = request.DebugName}), metadata);
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
