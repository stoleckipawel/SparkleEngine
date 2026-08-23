#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Core/Public/Diagnostics/Error.h"
#include "Pipeline/RasterPassRenderState.h"

FrameGraphRasterPass FrameGraph::BuildRasterPass(
    const PassParameterSet& parameters,
    const RasterPassRenderState& renderState) const
{
	const PassParameterLayout* const layout = parameters.GetLayout();
	if (layout == nullptr)
	{
		throw Diagnostics::Error("Raster pass has no parameter layout.");
	}

	FrameGraphRasterPass result;
	for (std::uint32_t index = 0; index < layout->GetParameterCount(); ++index)
	{
		const PassParameterDesc& parameter = layout->GetParameters()[index];
		if (parameter.Kind != ShaderParameterSemanticKind::RenderTarget
		    && parameter.Kind != ShaderParameterSemanticKind::DepthTarget)
		{
			continue;
		}

		const PassParameterBinding* const binding = parameters.GetBinding(index);
		const PassParameterTextureBindingData* const texture = binding != nullptr ? binding->AsTextureData() : nullptr;
		if (texture == nullptr || !texture->IsAttachment || texture->Handles.size() != 1
		    || !texture->Attachment.Handle.IsValid())
		{
			throw Diagnostics::Error("Raster pass attachment is not fully declared through the graph.");
		}
		if (texture->Attachment.Store == FrameGraphAttachmentStoreAction::Discard)
		{
			throw Diagnostics::Error("Raster attachment discard is not supported by the current graph execution contract.");
		}

		const FrameGraphResourceHandle resource = texture->Attachment.Handle.GetResourceHandle();
		if (!m_resourceRegistry.IsRegistered(resource))
		{
			throw Diagnostics::Error("Raster pass attachment is not registered in the frame graph.");
		}
		const FrameGraphTextureDesc& desc = m_resourceRegistry.GetMetadata(resource).textureDesc;
		if (desc.format == PixelFormat::Unknown || desc.sampleCount == 0)
		{
			throw Diagnostics::Error("Raster pass attachment has no valid format or sample count.");
		}
		if (parameter.Kind == ShaderParameterSemanticKind::RenderTarget)
		{
			if (desc.kind != FrameGraphTextureKind::Color || result.ColorCount >= result.Colors.size())
			{
				throw Diagnostics::Error("Raster color attachment has an incompatible graph texture description.");
			}
			result.Colors[result.ColorCount] = texture->Attachment;
			result.Compatibility.ColorFormats[result.ColorCount] = desc.format;
			++result.ColorCount;
			continue;
		}

		if (result.HasDepthStencil || desc.kind != FrameGraphTextureKind::DepthStencil)
		{
			throw Diagnostics::Error("Raster pass must declare at most one compatible depth-stencil attachment.");
		}
		result.DepthStencil = texture->Attachment;
		result.HasDepthStencil = true;
		result.Compatibility.DepthStencilAttachmentFormat = desc.format;
	}

	result.Compatibility.ColorCount = result.ColorCount;
	result.Compatibility.SampleCount = 1;
	for (std::uint32_t index = 0; index < result.ColorCount; ++index)
	{
		const FrameGraphTextureDesc& desc =
		    m_resourceRegistry.GetMetadata(result.Colors[index].Handle.GetResourceHandle()).textureDesc;
		if (index == 0)
		{
			result.Compatibility.SampleCount = desc.sampleCount;
		}
		else if (desc.sampleCount != result.Compatibility.SampleCount)
		{
			throw Diagnostics::Error("Raster color attachments use incompatible sample counts.");
		}
	}
	if (result.HasDepthStencil)
	{
		const FrameGraphTextureDesc& depthDesc =
		    m_resourceRegistry.GetMetadata(result.DepthStencil.Handle.GetResourceHandle()).textureDesc;
		if ((result.ColorCount != 0 && depthDesc.sampleCount != result.Compatibility.SampleCount)
		    || (renderState.GetDepth().DepthWriteEnable
	        && result.DepthStencil.DepthStencilAccess != FrameGraphDepthStencilAccess::ReadWrite))
		{
			throw Diagnostics::Error("Raster depth attachment is incompatible with pass depth or sample state.");
		}
		result.Compatibility.SampleCount = depthDesc.sampleCount;
	}
	else if (renderState.GetDepth().DepthEnable || renderState.GetStencil().StencilEnable)
	{
		throw Diagnostics::Error("Raster depth or stencil state requires a graph depth-stencil attachment.");
	}
	if (result.Compatibility.SampleCount != 1 && result.Compatibility.SampleCount != 2
	    && result.Compatibility.SampleCount != 4 && result.Compatibility.SampleCount != 8)
	{
		throw Diagnostics::Error("Raster pass attachments use an unsupported sample count.");
	}

	if (result.ColorCount == 0 && !result.HasDepthStencil)
	{
		throw Diagnostics::Error("Raster pass requires at least one graph attachment.");
	}
	return result;
}
