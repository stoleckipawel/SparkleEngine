#pragma once

#include "GameFramework/Public/Scene/Animations/SceneAnimation.h"

#include <cstdint>

namespace SceneAnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip) noexcept;

	void LogLoadedClip(const SceneAnimationClipDesc& clip);
	void LogUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip, std::uint32_t unsupportedRuntimeChannelCount);
}
