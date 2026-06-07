#include "PCH.h"

#include "Scene/Animations/SceneMorphWeightEvaluator.h"

#include "Scene/Animations/SceneAnimationSampler.h"

namespace SceneMorphWeightEvaluator
{
	void AppendSnapshots(
	    const SceneAnimationClipDesc& clip,
	    float playbackTimeSeconds,
	    std::vector<SceneMorphWeightSnapshot>& outMorphWeights)
	{
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			if (channel.targetPath != Assets::CookedAnimationTargetPath::Weights)
			{
				continue;
			}

			const DirectX::XMVECTOR sampledWeights = SceneAnimationSampler::SampleVectorChannel(clip, channel, playbackTimeSeconds);
			DirectX::XMFLOAT4 storedWeights{};
			DirectX::XMStoreFloat4(&storedWeights, sampledWeights);
			outMorphWeights.push_back(
			    SceneMorphWeightSnapshot{
			        .targetNodeIndex = channel.targetNodeIndex,
			        .weights = {storedWeights.x, storedWeights.y, storedWeights.z, storedWeights.w}});
		}
	}
}
