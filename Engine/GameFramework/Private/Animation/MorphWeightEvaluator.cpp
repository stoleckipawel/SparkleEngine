#include "PCH.h"

#include "Animation/MorphWeightEvaluator.h"

#include "Animation/AnimationSampler.h"

namespace MorphWeightEvaluator
{
	bool Evaluate(
	    const AnimationClipResource& clip,
	    std::uint32_t channelIndex,
	    float playbackTimeSeconds,
	    std::span<float> outputWeights) noexcept
	{
		if (channelIndex >= clip.channels.size() || outputWeights.size() != 4 ||
		    clip.channels[channelIndex].targetPath != Assets::CookedAnimationTargetPath::Weights)
			return false;
		DirectX::XMFLOAT4 sampled{};
		DirectX::XMStoreFloat4(
		    &sampled,
		    AnimationSampler::SampleVectorChannel(clip, clip.channels[channelIndex], playbackTimeSeconds));
		outputWeights[0] = sampled.x;
		outputWeights[1] = sampled.y;
		outputWeights[2] = sampled.z;
		outputWeights[3] = sampled.w;
		return true;
	}
}
