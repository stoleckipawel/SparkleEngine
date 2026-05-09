#include "PCH.h"

#include "Pipeline/Compression/CompressionPolicy.h"

#include "Pipeline/FormatPolicy.h"

#include <algorithm>

namespace TextureCookPipeline
{
	CompressionTarget ResolveCompressionTarget(const TextureCookRequest& request, const WorkingTexture& workingTexture) noexcept
	{
		switch (request.textureGroup)
		{
			case TextureGroup::Default:
				return CompressionTarget::None;
			case TextureGroup::NormalMap:
				return CompressionTarget::BC5;
			case TextureGroup::HdrColor:
				return CompressionTarget::BC6H;
			case TextureGroup::Roughness:
			case TextureGroup::Metallic:
			case TextureGroup::AmbientOcclusion:
			case TextureGroup::SubsurfaceStrength:
				return IsGreyscaleLike(workingTexture) ? CompressionTarget::BC4 : CompressionTarget::None;
			case TextureGroup::Diffuse:
			case TextureGroup::Emissive:
			case TextureGroup::SubsurfaceColor:
			default:
				if (workingTexture.sourceWasFloat)
				{
					return CompressionTarget::BC6H;
				}

				return HasMeaningfulAlpha(workingTexture) ? CompressionTarget::None : CompressionTarget::BC1;
		}
	}

	DXGI_FORMAT ResolveCompressedOutputFormat(
	    const TextureCookRequest& request,
	    const WorkingTexture& workingTexture,
	    CompressionTarget target) noexcept
	{
		switch (target)
		{
			case CompressionTarget::BC1:
				return request.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_BC1_UNORM_SRGB : DXGI_FORMAT_BC1_UNORM;
			case CompressionTarget::BC4:
				return DXGI_FORMAT_BC4_UNORM;
			case CompressionTarget::BC5:
				return DXGI_FORMAT_BC5_UNORM;
			case CompressionTarget::BC6H:
				return DXGI_FORMAT_BC6H_UF16;
			case CompressionTarget::BC7:
				if (request.textureGroup == TextureGroup::Roughness || request.textureGroup == TextureGroup::Metallic ||
				    request.textureGroup == TextureGroup::AmbientOcclusion || request.textureGroup == TextureGroup::SubsurfaceStrength)
				{
					return DXGI_FORMAT_BC7_UNORM;
				}

				return request.colorSpace == TextureColorSpace::Srgb ? DXGI_FORMAT_BC7_UNORM_SRGB : DXGI_FORMAT_BC7_UNORM;
			case CompressionTarget::None:
			default:
				return ResolveUncompressedOutputFormat(request, workingTexture.sourceWasFloat);
		}
	}

	std::uint32_t ComputeBlockCompressedRowPitch(CompressionTarget target, std::uint32_t width) noexcept
	{
		const std::uint32_t blockBytes = target == CompressionTarget::BC1 || target == CompressionTarget::BC4 ? 8u : 16u;
		return (std::max) (1u, (width + 3u) / 4u) * blockBytes;
	}

	std::uint32_t ComputeBlockCompressedSlicePitch(CompressionTarget target, std::uint32_t width, std::uint32_t height) noexcept
	{
		return ComputeBlockCompressedRowPitch(target, width) * (std::max) (1u, (height + 3u) / 4u);
	}
}