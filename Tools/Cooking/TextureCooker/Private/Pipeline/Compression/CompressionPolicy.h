#pragma once

#include "Pipeline/WorkingTexture.h"
#include "TextureCookRequestList.h"

#include <cstdint>

namespace TextureCookPipeline
{
	enum class CompressionTarget : std::uint8_t
	{
		None,
		BC1,
		BC4,
		BC5,
		BC6H,
		BC7,
	};

	CompressionTarget ResolveCompressionTarget(const TextureCookRequest& request, const WorkingTexture& workingTexture) noexcept;
	DXGI_FORMAT ResolveCompressedOutputFormat(
	    const TextureCookRequest& request,
	    const WorkingTexture& workingTexture,
	    CompressionTarget target) noexcept;

	std::uint32_t ComputeBlockCompressedRowPitch(CompressionTarget target, std::uint32_t width) noexcept;
	std::uint32_t ComputeBlockCompressedSlicePitch(CompressionTarget target, std::uint32_t width, std::uint32_t height) noexcept;
}
