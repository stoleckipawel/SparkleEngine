#pragma once

#include "RHI/Public/Interop/RhiNativeHandles.h"

#include <cstdint>

struct ImageProviderFrameInput;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl.h>

// Owns the common Streamline evaluation sequence shared by image providers:
// frame-token acquisition, view constants, and feature dispatch.
class StreamlineFrameEvaluation final
{
public:
	StreamlineFrameEvaluation(sl::ViewportHandle viewport, NativeGraphicsCommandListHandle commandList) noexcept;

	bool AcquireFrameToken(std::uint64_t frameId) noexcept;
	bool SetViewConstants(const ImageProviderFrameInput& frameInput) noexcept;
	bool Evaluate(sl::Feature feature) noexcept;

	const sl::FrameToken& GetFrameToken() const noexcept { return *m_frameToken; }

private:
	sl::FrameToken* m_frameToken = nullptr;
	sl::ViewportHandle m_viewport = {};
	sl::CommandBuffer* m_commandBuffer = nullptr;
};
#endif
