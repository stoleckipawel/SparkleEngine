#include "PCH.h"

#include "Pipeline/TexturePipeline.h"

#include "Pipeline/FormatPolicy.h"
#include "Pipeline/Stages/ChannelStage.h"
#include "Pipeline/Stages/DecodeStage.h"
#include "Pipeline/Stages/EncodeStage.h"
#include "Pipeline/Stages/MipStage.h"
#include "Pipeline/Stages/ShapeStage.h"

TextureLoadResult TexturePipeline::Process(
    const TextureCookRequest& request,
    TextureLoadResult sourceTexture)
{
	if (TextureCookPipeline::IsCompressedFormat(sourceTexture.dxgiFormat))
	{
		return TextureCookPipeline::ProcessCompressedSource(request, std::move(sourceTexture));
	}

	TextureCookPipeline::WorkingTexture workingTexture =
	    TextureCookPipeline::BuildWorkingTexture(request, sourceTexture);
	TextureCookPipeline::ApplyChannelPolicy(request, workingTexture);
	TextureCookPipeline::ApplyShapePolicy(request, workingTexture);
	TextureCookPipeline::ApplyMipPolicy(request, workingTexture);
	return TextureCookPipeline::BuildOutputTexture(request, workingTexture);
}
