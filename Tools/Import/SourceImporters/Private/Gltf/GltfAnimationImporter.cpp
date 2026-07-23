#include "PCH.h"

#include "Gltf/GltfAnimationImporter.h"

#include <cgltf.h>

#include <algorithm>
#include <format>
#include <limits>
#include <utility>

class GltfAnimationImporterOperations final
{
  public:
	static ImportedAnimationInterpolation ToImportedInterpolation(cgltf_interpolation_type interpolation) noexcept
	{
		switch (interpolation)
		{
			case cgltf_interpolation_type_step:
				return ImportedAnimationInterpolation::Step;
			case cgltf_interpolation_type_cubic_spline:
				return ImportedAnimationInterpolation::CubicSpline;
			case cgltf_interpolation_type_linear:
			default:
				return ImportedAnimationInterpolation::Linear;
		}
	}

	static ImportedAnimationTargetPath ToImportedTargetPath(cgltf_animation_path_type path) noexcept
	{
		switch (path)
		{
			case cgltf_animation_path_type_translation:
				return ImportedAnimationTargetPath::Translation;
			case cgltf_animation_path_type_rotation:
				return ImportedAnimationTargetPath::Rotation;
			case cgltf_animation_path_type_scale:
				return ImportedAnimationTargetPath::Scale;
			case cgltf_animation_path_type_weights:
				return ImportedAnimationTargetPath::Weights;
			default:
				return ImportedAnimationTargetPath::Unknown;
		}
	}

	static std::uint32_t GetValueComponentCount(ImportedAnimationTargetPath targetPath) noexcept
	{
		switch (targetPath)
		{
			case ImportedAnimationTargetPath::Translation:
			case ImportedAnimationTargetPath::Scale:
				return 3;
			case ImportedAnimationTargetPath::Rotation:
				return 4;
			case ImportedAnimationTargetPath::Weights:
			case ImportedAnimationTargetPath::Unknown:
			default:
				return 4;
		}
	}

	static DirectX::XMFLOAT4 ReadOutputValue(const cgltf_accessor* accessor, std::size_t index, std::uint32_t componentCount) noexcept
	{
		DirectX::XMFLOAT4 value{};
		if (accessor == nullptr || index >= accessor->count)
		{
			return value;
		}

		cgltf_float values[4] = {};
		cgltf_accessor_read_float(accessor, index, values, componentCount);
		value.x = values[0];
		value.y = values[1];
		value.z = values[2];
		value.w = componentCount > 3 ? values[3] : 0.0f;
		return value;
	}

	static float ReadInputTime(const cgltf_accessor* accessor, std::size_t index) noexcept
	{
		if (accessor == nullptr || index >= accessor->count)
		{
			return 0.0f;
		}

		cgltf_float value = 0.0f;
		cgltf_accessor_read_float(accessor, index, &value, 1);
		return value;
	}

	static std::uint32_t FindSamplerIndex(const cgltf_animation& animation, const cgltf_animation_sampler* sampler) noexcept
	{
		if (sampler == nullptr)
		{
			return (std::numeric_limits<std::uint32_t>::max)();
		}

		for (cgltf_size samplerIndex = 0; samplerIndex < animation.samplers_count; ++samplerIndex)
		{
			if (&animation.samplers[samplerIndex] == sampler)
			{
				return static_cast<std::uint32_t>(samplerIndex);
			}
		}

		return (std::numeric_limits<std::uint32_t>::max)();
	}

	static std::pair<std::uint32_t, std::uint32_t> FindSkeletonJointForNode(const SourceImportResult& result, std::uint32_t sourceNodeIndex) noexcept
	{
		for (std::size_t skeletonIndex = 0; skeletonIndex < result.scene.skeletons.size(); ++skeletonIndex)
		{
			const ImportedSkeleton& skeleton = result.scene.skeletons[skeletonIndex];
			for (std::size_t jointIndex = 0; jointIndex < skeleton.joints.size(); ++jointIndex)
			{
				if (skeleton.joints[jointIndex].sourceNodeIndex == sourceNodeIndex)
				{
					return {static_cast<std::uint32_t>(skeletonIndex), static_cast<std::uint32_t>(jointIndex)};
				}
			}
		}

		return {(std::numeric_limits<std::uint32_t>::max)(), (std::numeric_limits<std::uint32_t>::max)()};
	}

	static ImportedAnimationSampler ImportSampler(
	    const cgltf_animation_sampler& sampler,
	    ImportedAnimationTargetPath targetPath,
	    float& inOutClipDurationSeconds)
	{
		ImportedAnimationSampler importedSampler;
		importedSampler.interpolation = ToImportedInterpolation(sampler.interpolation);
		const cgltf_accessor* input = sampler.input;
		const cgltf_accessor* output = sampler.output;
		if (input == nullptr || output == nullptr || input->count == 0)
		{
			return importedSampler;
		}

		const std::uint32_t componentCount = GetValueComponentCount(targetPath);
		importedSampler.keyframes.reserve(input->count);
		const bool cubicSpline = importedSampler.interpolation == ImportedAnimationInterpolation::CubicSpline;
		for (cgltf_size keyframeIndex = 0; keyframeIndex < input->count; ++keyframeIndex)
		{
			const std::size_t outputBaseIndex = cubicSpline ? static_cast<std::size_t>(keyframeIndex * 3u) : static_cast<std::size_t>(keyframeIndex);
			if (outputBaseIndex >= output->count)
			{
				break;
			}

			ImportedAnimationKeyframe keyframe;
			keyframe.timeSeconds = ReadInputTime(input, keyframeIndex);
			if (cubicSpline)
			{
				keyframe.inTangent = ReadOutputValue(output, outputBaseIndex, componentCount);
				keyframe.value = ReadOutputValue(output, outputBaseIndex + 1u, componentCount);
				keyframe.outTangent = ReadOutputValue(output, outputBaseIndex + 2u, componentCount);
			}
			else
			{
				keyframe.value = ReadOutputValue(output, outputBaseIndex, componentCount);
			}

			inOutClipDurationSeconds = (std::max)(inOutClipDurationSeconds, keyframe.timeSeconds);
			importedSampler.keyframes.push_back(keyframe);
		}

		return importedSampler;
	}
};

void GltfAnimationImporter::ImportAnimations(const cgltf_data* data, SourceImportResult& result)
{
	if (data == nullptr || data->animations_count == 0)
	{
		return;
	}

	result.scene.animations.reserve(data->animations_count);
	for (cgltf_size animationIndex = 0; animationIndex < data->animations_count; ++animationIndex)
	{
		const cgltf_animation& animation = data->animations[animationIndex];
		ImportedAnimationClip clip;
		clip.name = animation.name != nullptr ? animation.name : std::format("Animation {}", animationIndex);
		clip.sourceAnimationIndex = static_cast<std::uint32_t>(animationIndex);
		clip.samplers.resize(animation.samplers_count);
		clip.channels.reserve(animation.channels_count);

		for (cgltf_size channelIndex = 0; channelIndex < animation.channels_count; ++channelIndex)
		{
			const cgltf_animation_channel& channel = animation.channels[channelIndex];
			const ImportedAnimationTargetPath targetPath = GltfAnimationImporterOperations::ToImportedTargetPath(channel.target_path);
			const std::uint32_t samplerIndex = GltfAnimationImporterOperations::FindSamplerIndex(animation, channel.sampler);
			if (targetPath == ImportedAnimationTargetPath::Unknown || samplerIndex >= clip.samplers.size() || channel.target_node == nullptr)
			{
				continue;
			}

			if (!clip.samplers[samplerIndex].IsValid())
			{
				clip.samplers[samplerIndex] = GltfAnimationImporterOperations::ImportSampler(*channel.sampler, targetPath, clip.durationSeconds);
			}

			if (!clip.samplers[samplerIndex].IsValid())
			{
				continue;
			}

			const std::uint32_t targetNodeIndex = static_cast<std::uint32_t>(cgltf_node_index(data, channel.target_node));
			const auto [targetSkeletonIndex, targetJointIndex] = GltfAnimationImporterOperations::FindSkeletonJointForNode(result, targetNodeIndex);
			if (clip.targetSkeletonIndex == (std::numeric_limits<std::uint32_t>::max)())
			{
				clip.targetSkeletonIndex = targetSkeletonIndex;
			}

			clip.channels.push_back(
			    ImportedAnimationChannel{
			        .targetPath = targetPath,
			        .targetNodeIndex = targetNodeIndex,
			        .targetJointIndex = targetJointIndex,
			        .samplerIndex = samplerIndex});
		}

		if (clip.IsValid())
		{
			result.scene.animations.push_back(std::move(clip));
		}
	}

}
