#include "PCH.h"

#include "Animation/AnimationDiagnostics.h"

static const auto g_animationEvaluationLogger = Logging::GetOrCreateLogger("GameFramework.AnimationEvaluation");

namespace AnimationDiagnostics
{
	std::uint32_t CountUnsupportedRuntimeChannels(const AnimationClipResource& clip) noexcept
	{
		std::uint32_t count = 0;
		for (const AnimationChannel& channel : clip.channels)
		{
			if (channel.targetPath == Assets::CookedAnimationTargetPath::Unknown)
				++count;
		}
		return count;
	}

	void LogLoadedClip(const AnimationClipResource& clip)
	{
		static_cast<void>(clip);
	}

	void LogUnsupportedRuntimeChannels(const AnimationClipResource& clip, std::uint32_t unsupportedRuntimeChannelCount)
	{
		SPDLOG_LOGGER_WARN(
		    g_animationEvaluationLogger,
		    "Animation evaluation: clip '{}' has {} unsupported runtime animation channel(s).",
		    clip.name,
		    unsupportedRuntimeChannelCount);
	}
}
