#include "PCH.h"

#include "FrameGraph/Builder/FrameGraphCopyPasses.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/Execution/PassCommandContext.h"

namespace FrameGraphCopyPasses
{
	void AddTextureCopy(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphTextureHandle destination,
	    FrameGraphTextureHandle source)
	{
		builder.AddPass(
		    name,
		    EFrameGraphPassKind::Transfer,
		    EFrameGraphQueuePreference::Copy,
		    [destination, source](PassResourceBuilder& resources)
		    {
			    resources.Read(source, ResourceUsage::CopySource, "Source");
			    resources.Write(destination, ResourceUsage::CopyDest, "Destination");
		    },
		    [destination, source](PassCommandContext& context) { context.Resources.CopyTexture(context.Commands, destination, source); });
	}

	void AddBufferCopy(FrameGraphBuilder& builder, std::string_view name, FrameGraphBufferHandle destination, FrameGraphBufferHandle source)
	{
		builder.AddPass(
		    name,
		    EFrameGraphPassKind::Transfer,
		    EFrameGraphQueuePreference::Copy,
		    [destination, source](PassResourceBuilder& resources)
		    {
			    resources.Read(source, ResourceUsage::CopySource, "Source");
			    resources.Write(destination, ResourceUsage::CopyDest, "Destination");
		    },
		    [destination, source](PassCommandContext& context) { context.Resources.CopyBuffer(context.Commands, destination, source); });
	}
}
