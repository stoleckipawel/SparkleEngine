#include "PCH.h"

#include "Pipeline/Stages/ShapeStage.h"

#include "Core/Public/Diagnostics/Error.h"

namespace TextureCookPipeline
{
	void ApplyShapePolicy(const TextureCookRequest& request, WorkingTexture& workingTexture)
	{
		if (request.policy.dimension == TextureDimension::Texture2D)
		{
			if (workingTexture.dimension != TextureResourceDimension::Texture2D)
			{
				throw Diagnostics::Error("Cannot cook cubemap source content as a 2D texture.");
			}

			return;
		}

		if (workingTexture.dimension == TextureResourceDimension::TextureCube)
		{
			return;
		}

		throw Diagnostics::Error("TextureCube cook requests need source texture data that is already a cubemap.");
	}
}
