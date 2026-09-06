#pragma once

#include "Animation/AnimationClipResource.h"

#include <cstdint>

namespace AnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const AnimationClipResource& clip) noexcept;
	void LogLoadedClip(const AnimationClipResource& clip);
	void LogUnsupportedRuntimeChannels(const AnimationClipResource& clip, std::uint32_t unsupportedRuntimeChannelCount);
}
