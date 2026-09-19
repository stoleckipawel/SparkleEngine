#include "../../PCH.h"
#include "Passes/Presentation/PresentationPasses.h"

#include "Passes/Presentation/OutputEncoding.h"
#include "Passes/Presentation/PresentationOutput.h"
#include "Passes/Presentation/ToneMapping.h"

void AddPresentationPasses(FrameGraphBuilder& builder, const RenderFrameGraphSettings& settings, RenderFrameGraphResources& resources)
{
	const FrameGraphTextureHandle toneMappedColor = AddToneMappingPass(builder, settings.OutputExtent, resources);
	const FrameGraphTextureHandle encodedColor = AddOutputEncodingPass(builder, settings, toneMappedColor);
	AddPresentationOutputPass(builder, settings, encodedColor, resources);
}
