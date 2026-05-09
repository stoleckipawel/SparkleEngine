#include "PCH.h"

#include "Pipeline/TexturePipeline.h"

#include "Pipeline/FormatPolicy.h"
#include "Pipeline/Stages/ChannelStage.h"
#include "Pipeline/Stages/DecodeStage.h"
#include "Pipeline/Stages/EncodeStage.h"
#include "Pipeline/Stages/MipStage.h"
#include "Pipeline/Stages/ShapeStage.h"

	bool TexturePipeline::Process(
	    const TextureCookRequest& request,
	    TextureLoadResult sourceTexture,
	    TextureLoadResult& outProcessedTexture,
	    std::string& outErrorMessage)
	{
		if (!sourceTexture.IsValid())
		{
			outErrorMessage = "Source texture payload is invalid.";
			return false;
		}

		if (TextureCookPipeline::IsCompressedFormat(sourceTexture.dxgiFormat))
		{
			return TextureCookPipeline::ProcessCompressedSource(request, std::move(sourceTexture), outProcessedTexture, outErrorMessage);
		}

		TextureCookPipeline::WorkingTexture workingTexture;
		if (!TextureCookPipeline::BuildWorkingTexture(request, sourceTexture, workingTexture, outErrorMessage))
		{
			return false;
		}

		if (!TextureCookPipeline::ApplyChannelPolicy(request, workingTexture, outErrorMessage))
		{
			return false;
		}

		if (!TextureCookPipeline::ApplyShapePolicy(request, workingTexture, outErrorMessage))
		{
			return false;
		}

		if (!TextureCookPipeline::ApplyMipPolicy(request, workingTexture, outErrorMessage))
		{
			return false;
		}

		return TextureCookPipeline::BuildOutputTexture(request, workingTexture, outProcessedTexture, outErrorMessage);
	}
