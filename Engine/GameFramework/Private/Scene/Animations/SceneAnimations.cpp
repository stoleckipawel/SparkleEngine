#include "PCH.h"

#include "Scene/Animations/SceneAnimations.h"

#include "Scene/Skeletons/SceneSkeletons.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

static const auto g_sceneAnimationsLogger = Logging::GetOrCreateLogger("GameFramework.SceneAnimations");

namespace
{
	struct JointLocalTransform final
	{
		DirectX::XMVECTOR translation = DirectX::XMVectorZero();
		DirectX::XMVECTOR rotation = DirectX::XMQuaternionIdentity();
		DirectX::XMVECTOR scale = DirectX::XMVectorSet(1.0f, 1.0f, 1.0f, 0.0f);
	};

	DirectX::XMMATRIX LoadMatrix(const DirectX::XMFLOAT4X4& matrix) noexcept
	{
		return DirectX::XMLoadFloat4x4(&matrix);
	}

	JointLocalTransform DecomposeTransform(DirectX::FXMMATRIX matrix) noexcept
	{
		JointLocalTransform transform;
		if (!DirectX::XMMatrixDecompose(&transform.scale, &transform.rotation, &transform.translation, matrix))
		{
			transform = {};
		}
		transform.rotation = DirectX::XMQuaternionNormalize(transform.rotation);
		return transform;
	}

	DirectX::XMMATRIX ComposeTransform(const JointLocalTransform& transform) noexcept
	{
		return DirectX::XMMatrixScalingFromVector(transform.scale) *
		       DirectX::XMMatrixRotationQuaternion(transform.rotation) *
		       DirectX::XMMatrixTranslationFromVector(transform.translation);
	}

	std::vector<JointLocalTransform> BuildBindLocalTransforms(const SceneSkeletonDesc& skeleton)
	{
		std::vector<JointLocalTransform> localTransforms;
		localTransforms.reserve(skeleton.joints.size());
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
		{
			const SceneJointDesc& joint = skeleton.joints[jointIndex];
			DirectX::XMMATRIX localMatrix = LoadMatrix(joint.bindPoseWorldTransform);
			if (joint.parentJointIndex < skeleton.joints.size())
			{
				const DirectX::XMMATRIX parentWorld = LoadMatrix(skeleton.joints[joint.parentJointIndex].bindPoseWorldTransform);
				localMatrix = localMatrix * DirectX::XMMatrixInverse(nullptr, parentWorld);
			}
			localTransforms.push_back(DecomposeTransform(localMatrix));
		}
		return localTransforms;
	}

	DirectX::XMVECTOR LoadValue(const DirectX::XMFLOAT4& value) noexcept
	{
		return DirectX::XMLoadFloat4(&value);
	}

	std::uint32_t FindKeyframeSegment(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		if (channel.keyframeCount <= 1u)
		{
			return 0;
		}

		const std::uint32_t first = channel.firstKeyframe;
		const std::uint32_t lastSegment = channel.keyframeCount - 2u;
		for (std::uint32_t segment = 0; segment <= lastSegment; ++segment)
		{
			const SceneAnimationKeyframe& nextKey = clip.keyframes[first + segment + 1u];
			if (timeSeconds <= nextKey.timeSeconds)
			{
				return segment;
			}
		}

		return lastSegment;
	}

	float ComputeSegmentAlpha(const SceneAnimationKeyframe& lhs, const SceneAnimationKeyframe& rhs, float timeSeconds) noexcept
	{
		const float duration = rhs.timeSeconds - lhs.timeSeconds;
		if (duration <= (std::numeric_limits<float>::epsilon)())
		{
			return 0.0f;
		}

		return std::clamp((timeSeconds - lhs.timeSeconds) / duration, 0.0f, 1.0f);
	}

	DirectX::XMVECTOR SampleVectorChannel(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMVectorZero();
		}

		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value);
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const SceneAnimationKeyframe& lhs = clip.keyframes[first + segment];
		const SceneAnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return DirectX::XMVectorLerp(LoadValue(lhs.value), LoadValue(rhs.value), alpha);
	}

	DirectX::XMVECTOR SampleRotationChannel(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds) noexcept
	{
		const std::uint32_t first = channel.firstKeyframe;
		if (channel.keyframeCount == 0u || first >= clip.keyframes.size())
		{
			return DirectX::XMQuaternionIdentity();
		}

		if (channel.keyframeCount == 1u || channel.interpolation == Assets::CookedAnimationInterpolation::Step)
		{
			return DirectX::XMQuaternionNormalize(LoadValue(clip.keyframes[first + FindKeyframeSegment(clip, channel, timeSeconds)].value));
		}

		const std::uint32_t segment = FindKeyframeSegment(clip, channel, timeSeconds);
		const SceneAnimationKeyframe& lhs = clip.keyframes[first + segment];
		const SceneAnimationKeyframe& rhs = clip.keyframes[first + segment + 1u];
		const float alpha = ComputeSegmentAlpha(lhs, rhs, timeSeconds);
		return DirectX::XMQuaternionNormalize(DirectX::XMQuaternionSlerp(LoadValue(lhs.value), LoadValue(rhs.value), alpha));
	}

	void ApplyChannel(const SceneAnimationClipDesc& clip, const SceneAnimationChannel& channel, float timeSeconds, std::vector<JointLocalTransform>& localTransforms)
	{
		if (channel.targetJointIndex >= localTransforms.size())
		{
			return;
		}

		JointLocalTransform& transform = localTransforms[channel.targetJointIndex];
		switch (channel.targetPath)
		{
			case Assets::CookedAnimationTargetPath::Translation:
				transform.translation = SampleVectorChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Rotation:
				transform.rotation = SampleRotationChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Scale:
				transform.scale = SampleVectorChannel(clip, channel, timeSeconds);
				break;
			case Assets::CookedAnimationTargetPath::Weights:
			case Assets::CookedAnimationTargetPath::Unknown:
			default:
				break;
		}
	}

	void ComposeModelSpaceTransforms(
	    const SceneSkeletonDesc& skeleton,
	    const std::vector<JointLocalTransform>& localTransforms,
	    std::vector<DirectX::XMMATRIX>& outModelSpaceTransforms)
	{
		outModelSpaceTransforms.resize(localTransforms.size(), DirectX::XMMatrixIdentity());
		for (std::size_t jointIndex = 0; jointIndex < localTransforms.size(); ++jointIndex)
		{
			DirectX::XMMATRIX modelMatrix = ComposeTransform(localTransforms[jointIndex]);
			const std::uint32_t parentJointIndex = skeleton.joints[jointIndex].parentJointIndex;
			if (parentJointIndex < outModelSpaceTransforms.size())
			{
				modelMatrix = modelMatrix * outModelSpaceTransforms[parentJointIndex];
			}
			outModelSpaceTransforms[jointIndex] = modelMatrix;
		}
	}

	SceneAnimationPoseSnapshot EvaluateClip(
	    const SceneAnimationClipDesc& clip,
	    const SceneSkeletonDesc& skeleton,
	    float playbackTimeSeconds)
	{
		std::vector<JointLocalTransform> localTransforms = BuildBindLocalTransforms(skeleton);
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			ApplyChannel(clip, channel, playbackTimeSeconds, localTransforms);
		}

		std::vector<DirectX::XMMATRIX> modelSpaceTransforms;
		ComposeModelSpaceTransforms(skeleton, localTransforms, modelSpaceTransforms);

		SceneAnimationPoseSnapshot pose;
		pose.skeletonAssetId = skeleton.assetId;
		pose.animationAssetId = clip.animationAssetId;
		pose.clipName = clip.name;
		pose.playbackTimeSeconds = playbackTimeSeconds;
		pose.jointCount = static_cast<std::uint32_t>(skeleton.joints.size());
		pose.skinningMatrices.reserve(skeleton.joints.size());
		for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
		{
			const DirectX::XMMATRIX skinningMatrix = LoadMatrix(skeleton.joints[jointIndex].inverseBindMatrix) * modelSpaceTransforms[jointIndex];
			DirectX::XMFLOAT4X4 storedMatrix{};
			DirectX::XMStoreFloat4x4(&storedMatrix, skinningMatrix);
			pose.skinningMatrices.push_back(storedMatrix);
		}

		return pose;
	}

	std::uint32_t CountUnsupportedRuntimeChannels(const SceneAnimationClipDesc& clip) noexcept
	{
		std::uint32_t unsupportedChannelCount = 0;
		for (const SceneAnimationChannel& channel : clip.channels)
		{
			if (channel.targetPath == Assets::CookedAnimationTargetPath::Weights ||
			    channel.targetPath == Assets::CookedAnimationTargetPath::Unknown)
			{
				++unsupportedChannelCount;
			}
		}
		return unsupportedChannelCount;
	}
}

void SceneAnimations::Clear() noexcept
{
	m_clips.clear();
	m_playbackStates.clear();
	m_activePoses.clear();
	m_playbackDiagnosticLogCount = 0;
}

void SceneAnimations::AppendClips(std::vector<SceneAnimationClipDesc>&& clips)
{
	m_clips.reserve(m_clips.size() + clips.size());
	m_playbackStates.reserve(m_playbackStates.size() + clips.size());
	for (SceneAnimationClipDesc& clip : clips)
	{
		const std::uint32_t unsupportedRuntimeChannelCount = CountUnsupportedRuntimeChannels(clip);
		SPDLOG_LOGGER_INFO(
		    g_sceneAnimationsLogger,
		    "SceneAnimations: loaded clip '{}' duration={:.3f}s channels={} keyframes={} animationAsset=0x{:016X} targetSkeleton=0x{:016X}",
		    clip.name,
		    clip.durationSeconds,
		    clip.channelCount,
		    clip.keyframeCount,
		    clip.animationAssetId,
		    clip.targetSkeletonAssetId);
		if (unsupportedRuntimeChannelCount > 0u)
		{
			SPDLOG_LOGGER_WARN(
			    g_sceneAnimationsLogger,
			    "SceneAnimations: clip '{}' has {} unsupported runtime animation channel(s); Stage 6 playback applies translation, rotation, and scale channels.",
			    clip.name,
			    unsupportedRuntimeChannelCount);
		}
		m_clips.push_back(std::move(clip));
		m_playbackStates.push_back(PlaybackState{});
	}
}

void SceneAnimations::Update(float deltaSeconds, const SceneSkeletons& skeletons)
{
	m_activePoses.clear();
	m_activePoses.reserve(m_clips.size());

	for (std::size_t clipIndex = 0; clipIndex < m_clips.size(); ++clipIndex)
	{
		const SceneAnimationClipDesc& clip = m_clips[clipIndex];
		PlaybackState& playback = m_playbackStates[clipIndex];
		if (!playback.paused && clip.durationSeconds > 0.0f)
		{
			playback.playbackTimeSeconds += (std::max)(0.0f, deltaSeconds) * playback.speed;
			if (playback.looping)
			{
				playback.playbackTimeSeconds = std::fmod(playback.playbackTimeSeconds, clip.durationSeconds);
			}
			else
			{
				playback.playbackTimeSeconds = std::min(playback.playbackTimeSeconds, clip.durationSeconds);
			}
		}

		for (const SceneSkeletonDesc& skeleton : skeletons.GetSkeletons())
		{
			if (skeleton.assetId != clip.targetSkeletonAssetId)
			{
				continue;
			}

			m_activePoses.push_back(EvaluateClip(clip, skeleton, playback.playbackTimeSeconds));
			break;
		}
	}

	if (m_playbackDiagnosticLogCount < 3u && !m_activePoses.empty())
	{
		++m_playbackDiagnosticLogCount;
		SPDLOG_LOGGER_INFO(
		    g_sceneAnimationsLogger,
		    "SceneAnimations: playback active clips={} poses={} firstClip='{}' time={:.3f}s joints={}",
		    m_clips.size(),
		    m_activePoses.size(),
		    m_activePoses.front().clipName,
		    m_activePoses.front().playbackTimeSeconds,
		    m_activePoses.front().jointCount);
	}
}

SceneAnimationSnapshot SceneAnimations::CaptureSnapshot() const
{
	SceneAnimationSnapshot snapshot;
	snapshot.poses = m_activePoses;
	return snapshot;
}
