#include "PCH.h"

#include "Scene/Animations/SceneAnimationDiagnostics.h"

static const auto g_animationEvaluationLogger = Logging::GetOrCreateLogger("GameFramework.AnimationEvaluation");

namespace SceneAnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip) noexcept
	{
		std::uint32_t unsupportedChannelCount = 0;
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			if (channel.targetPath == Assets::CookedAnimationTargetPath::Unknown)
			{
				++unsupportedChannelCount;
			}
		}
		return unsupportedChannelCount;
	}

	void LogLoadedClip(const SceneAnimationClipDesc& clip)
	{
		static_cast<void>(clip);
	}

	void LogUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip, std::uint32_t unsupportedRuntimeChannelCount)
	{
		SPDLOG_LOGGER_WARN(
		    g_animationEvaluationLogger,
		    "Animation evaluation: clip '{}' has {} unsupported runtime animation channel(s); playback applies translation, rotation, scale, and skeletal morph-weight channels.",
		    clip.name,
		    unsupportedRuntimeChannelCount);
	}

}
