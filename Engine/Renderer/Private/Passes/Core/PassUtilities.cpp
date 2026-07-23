#include "PCH.h"

#include "Passes/Core/PassUtilities.h"

namespace PassUtilities
{
	void DrawFullscreenTriangle(RenderCommandContext& cmd) noexcept
	{
		cmd.SetPrimitiveTopology(RhiPrimitiveTopology::TriangleList);
		cmd.DrawInstanced(3, 1, 0, 0);
	}

	const PassParameterSet& GetEmptyPassParameterSet() noexcept
	{
		static const PassParameterLayout emptyLayout("PassUtilities.EmptyPassParameters");
		static const PassParameterSet emptyParameters(emptyLayout, {});
		return emptyParameters;
	}

	void AppendBindingNameIfCompiled(
	    std::vector<const char*>& bindingNames,
	    const RenderBindingLayout& bindingLayout,
	    const char* bindingName) noexcept
	{
		if (bindingName == nullptr || bindingName[0] == '\0' || bindingLayout.FindBinding(bindingName) == nullptr)
		{
			return;
		}

		const auto existing = std::ranges::find_if(
		    bindingNames,
		    [bindingName](const char* existingName)
		    {
			    return std::string_view(existingName != nullptr ? existingName : "") == bindingName;
		    });
		if (existing == bindingNames.end())
		{
			bindingNames.push_back(bindingName);
		}
	}

	std::vector<const char*> BuildBoundBindingNames(
	    const RenderBindingLayout& bindingLayout,
	    const PassParameterSet& parameters,
	    const PassBindingOverrides* overrides) noexcept
	{
		std::vector<const char*> bindingNames;
		if (const PassParameterLayout* parameterLayout = parameters.GetLayout())
		{
			const std::vector<PassParameterDesc>& parameterDescs = parameterLayout->GetParameters();
			for (std::size_t index = 0; index < parameterDescs.size() && index < parameters.GetBindingCount(); ++index)
			{
				const PassParameterBinding* binding = parameters.GetBinding(static_cast<std::uint32_t>(index));
				if (binding != nullptr && binding->IsBound())
				{
					AppendBindingNameIfCompiled(bindingNames, bindingLayout, parameterDescs[index].Name.c_str());
				}
			}
		}

		if (overrides != nullptr)
		{
			for (const PassBindingOverride& bindingOverride : overrides->GetOverrides())
			{
				AppendBindingNameIfCompiled(bindingNames, bindingLayout, bindingOverride.Name.c_str());
			}
		}

		return bindingNames;
	}

	void AddCopyTexturePass(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphTextureHandle destinationHandle,
	    FrameGraphTextureHandle sourceHandle)
	{
		builder.Execute(
		    name,
		    EFrameGraphPassKind::Transfer,
		    EFrameGraphQueuePreference::Copy,
		    [destinationHandle, sourceHandle](PassResourceBuilder& resourceBuilder)
		    {
			    resourceBuilder.Read(sourceHandle, ResourceUsage::CopySource, "Source");
			    resourceBuilder.Write(destinationHandle, ResourceUsage::CopyDest, "Destination");
		    },
		    [destinationHandle, sourceHandle](PassExecutionContext& context)
		    {
			    context.Resources.CopyTexture(context.Commands, destinationHandle, sourceHandle);
		    });
	}

	void AddCopyBufferPass(
	    FrameGraphBuilder& builder,
	    std::string_view name,
	    FrameGraphBufferHandle destinationHandle,
	    FrameGraphBufferHandle sourceHandle)
	{
		builder.Execute(
		    name,
		    EFrameGraphPassKind::Transfer,
		    EFrameGraphQueuePreference::Copy,
		    [destinationHandle, sourceHandle](PassResourceBuilder& resourceBuilder)
		    {
			    resourceBuilder.Read(sourceHandle, ResourceUsage::CopySource, "Source");
			    resourceBuilder.Write(destinationHandle, ResourceUsage::CopyDest, "Destination");
		    },
		    [destinationHandle, sourceHandle](PassExecutionContext& context)
		    {
			    context.Resources.CopyBuffer(context.Commands, destinationHandle, sourceHandle);
		    });
	}
}
