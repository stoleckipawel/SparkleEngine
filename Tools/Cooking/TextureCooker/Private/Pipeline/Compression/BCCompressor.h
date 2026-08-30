#pragma once

#include "Pipeline/Compression/CompressionPolicy.h"

namespace TextureCookPipeline
{
	class BCCompressor final
	{
	public:
		explicit BCCompressor(CompressionTarget target) noexcept;
		~BCCompressor();

		BCCompressor(const BCCompressor&) = delete;
		BCCompressor& operator=(const BCCompressor&) = delete;

		void Initialize(bool srgbOutput, bool imageNeedsAlpha);
		TextureMipLevelData CompressMip(const TextureCookRequest& request, const WorkingMipLevel& sourceMip) const;

	private:
		void Destroy() noexcept;

		CompressionTarget target_ = CompressionTarget::None;
		void* options_ = nullptr;
	};
}
