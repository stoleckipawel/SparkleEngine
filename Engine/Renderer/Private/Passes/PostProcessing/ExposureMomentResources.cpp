#include "../../PCH.h"
#include "Passes/PostProcessing/ExposureMomentResources.h"

#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraphTextureDesc.h"
#include "RHI/Public/Formats/PixelFormat.h"

#include <string>

ExposureMomentTexture CreateExposureMomentTexture(
    FrameGraphBuilder& builder,
    const char* prefix,
    std::uint32_t level,
    std::uint32_t width,
    std::uint32_t height)
{
	const std::string name = std::string(prefix) + std::to_string(level);
	return ExposureMomentTexture{
	    .Handle = builder.CreateTexture(
	        FrameGraphTextureDesc::CreateColor(name, width, height, PixelFormat::R32G32B32A32_Float)),
	    .Width = width,
	    .Height = height};
}
