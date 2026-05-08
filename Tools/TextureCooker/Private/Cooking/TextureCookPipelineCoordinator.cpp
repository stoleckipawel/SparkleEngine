#include "PCH.h"

#include "Cooking/TextureCookPipelineCoordinator.h"

#include "Cooking/Pipeline/TextureCookMipStage.h"
#include "Cooking/Pipeline/TextureCookOutputStage.h"
#include "Cooking/Pipeline/TextureCookPipelineUtils.h"
#include "Cooking/Pipeline/TextureCookDecodeStage.h"

namespace AssetAuthoring
{
	bool TextureCookPipelineCoordinator::Process(
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

		TextureCookPipeline::WorkingImage workingImage;
		if (!TextureCookPipeline::BuildWorkingImage(request, sourceTexture, workingImage, outErrorMessage))
		{
			return false;
		}

		if (!TextureCookPipeline::ApplyMipPolicy(request, workingImage, outErrorMessage))
		{
			return false;
		}

		return TextureCookPipeline::BuildOutputTexture(request, workingImage, outProcessedTexture, outErrorMessage);
	}
}
