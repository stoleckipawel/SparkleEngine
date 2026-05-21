#pragma once

#include "Pipeline/Compression/CompressionPolicy.h"

#include <string>

namespace TextureCookPipeline
{
	class BCCompressor final
	{
	  public:
		explicit BCCompressor(CompressionTarget target) noexcept;
		~BCCompressor();

		BCCompressor(const BCCompressor&) = delete;
		BCCompressor& operator=(const BCCompressor&) = delete;

		bool Initialize(bool srgbOutput, bool imageNeedsAlpha, std::string& outErrorMessage);
		bool CompressMip(
		    const TextureCookRequest& request,
		    const WorkingMipLevel& sourceMip,
		    TextureMipLevelData& outMip,
		    std::string& outErrorMessage) const;

	  private:
		void Destroy() noexcept;

		CompressionTarget target_ = CompressionTarget::None;
		void* options_ = nullptr;
	};
}