#include "PCH.h"

#include "Pipeline/Stages/ShapeStage.h"

namespace TextureCookPipeline
{
	bool ApplyShapePolicy(const TextureCookRequest& request, WorkingTexture& workingTexture, std::string& outErrorMessage)
	{
		if (request.dimension == TextureDimension::Texture2D)
		{
			if (workingTexture.dimension != TextureResourceDimension::Texture2D)
			{
				outErrorMessage = "Cannot cook cubemap source content as a 2D texture.";
				return false;
			}

			outErrorMessage.clear();
			return true;
		}

		if (workingTexture.dimension == TextureResourceDimension::TextureCube)
		{
			outErrorMessage.clear();
			return true;
		}

		outErrorMessage = "TextureCube cook requests require source texture data that is already a cubemap.";
		return false;
	}
}