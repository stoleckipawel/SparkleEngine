#pragma once

#include "Animation/AnimationClipResource.h"

namespace AnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const AnimationClipResource& clip) noexcept;
	void LogLoadedClip(const AnimationClipResource& clip);
	void LogUnsupportedRuntimeChannels(const AnimationClipResource& clip, std::uint32_t unsupportedRuntimeChannelCount);
}
