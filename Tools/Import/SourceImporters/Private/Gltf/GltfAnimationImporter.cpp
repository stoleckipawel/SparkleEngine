#include "PCH.h"

#include "Gltf/GltfAnimationImporter.h"
#include "Gltf/GltfCoordinateConverter.h"
#include "Core/Public/Diagnostics/Error.h"

#include <cgltf.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <string>
#include <utility>
#include <vector>

class GltfAnimationTranslation final
{
public:
	static ImportedAnimationInterpolation ToImportedInterpolation(cgltf_interpolation_type interpolation)
	{
		switch (interpolation)
		{
			case cgltf_interpolation_type_step:
				return ImportedAnimationInterpolation::Step;
			case cgltf_interpolation_type_cubic_spline:
				return ImportedAnimationInterpolation::CubicSpline;
			case cgltf_interpolation_type_linear:
				return ImportedAnimationInterpolation::Linear;
			default:
				throw Diagnostics::Error("glTF animation sampler uses an unsupported interpolation mode.");
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
				return 0;
		}
	}

	static DirectX::XMFLOAT4 ReadOutputValue(const cgltf_accessor* accessor, std::size_t index, std::uint32_t componentCount)
	{
		if (accessor == nullptr || index >= accessor->count || componentCount == 0 || componentCount > 4)
		{
			throw Diagnostics::Error(std::format("glTF animation output element {} is outside its accessor.", index));
		}

		cgltf_float values[4] = {};
		if (!cgltf_accessor_read_float(accessor, index, values, componentCount))
		{
			throw Diagnostics::Error(std::format("Cannot decode glTF animation output element {}.", index));
		}
		return {values[0], values[1], values[2], componentCount > 3 ? values[3] : 0.0f};
	}

	static DirectX::XMFLOAT4 ReadWeightOutputValue(const cgltf_accessor* accessor, std::size_t firstScalarIndex)
	{
		if (accessor == nullptr || firstScalarIndex > accessor->count || accessor->count - firstScalarIndex < 4u)
		{
			throw Diagnostics::Error(std::format("glTF animation weight element {} is outside its accessor.", firstScalarIndex));
		}

		DirectX::XMFLOAT4 value;
		float* outputComponents[4] = {&value.x, &value.y, &value.z, &value.w};
		for (std::size_t componentIndex = 0; componentIndex < 4u; ++componentIndex)
		{
			if (!cgltf_accessor_read_float(accessor, firstScalarIndex + componentIndex, outputComponents[componentIndex], 1))
			{
				throw Diagnostics::Error(std::format("Cannot decode glTF animation weight element {}.", firstScalarIndex + componentIndex));
			}
		}
		return value;
	}

	static float ReadInputTime(const cgltf_accessor* accessor, std::size_t index)
	{
		float time = 0.0f;
		if (accessor == nullptr || index >= accessor->count || !cgltf_accessor_read_float(accessor, index, &time, 1)
		    || !std::isfinite(time))
		{
			throw Diagnostics::Error(std::format("Cannot decode glTF animation input time {}.", index));
		}
		return time;
	}

	static bool IsFinite(const DirectX::XMFLOAT4& value) noexcept
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
	}

	static DirectX::XMFLOAT4 ConvertTranslationValue(const DirectX::XMFLOAT4& source, bool tangent) noexcept
	{
		const DirectX::XMFLOAT3 converted = tangent ? GltfCoordinateConverter::ConvertTranslationTangent({source.x, source.y, source.z})
		                                            : GltfCoordinateConverter::ConvertTranslation({source.x, source.y, source.z});
		return {converted.x, converted.y, converted.z, 0.0f};
	}

	static DirectX::XMFLOAT4 ConvertAnimationValue(
	    ImportedAnimationTargetPath targetPath,
	    const DirectX::XMFLOAT4& source,
	    bool tangent) noexcept
	{
		switch (targetPath)
		{
			case ImportedAnimationTargetPath::Translation:
				return ConvertTranslationValue(source, tangent);
			case ImportedAnimationTargetPath::Rotation:
				return tangent ? GltfCoordinateConverter::ConvertRotationTangent(source) : GltfCoordinateConverter::ConvertRotation(source);
			case ImportedAnimationTargetPath::Scale:
			case ImportedAnimationTargetPath::Weights:
			case ImportedAnimationTargetPath::Unknown:
			default:
				return source;
		}
	}

	static void NormalizeKeyframe(
	    ImportedAnimationTargetPath targetPath,
	    bool cubicSpline,
	    const DirectX::XMFLOAT4* previousRotation,
	    ImportedAnimationKeyframe& keyframe)
	{
		keyframe.value = ConvertAnimationValue(targetPath, keyframe.value, false);
		if (cubicSpline)
		{
			keyframe.inTangent = ConvertAnimationValue(targetPath, keyframe.inTangent, true);
			keyframe.outTangent = ConvertAnimationValue(targetPath, keyframe.outTangent, true);
		}
		if (!IsFinite(keyframe.value) || (cubicSpline && (!IsFinite(keyframe.inTangent) || !IsFinite(keyframe.outTangent))))
		{
			throw Diagnostics::Error("glTF animation contains a non-finite keyframe value or tangent.");
		}

		if (targetPath != ImportedAnimationTargetPath::Rotation)
		{
			return;
		}

		DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&keyframe.value);
		if (DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(rotation)) <= 1.0e-8f)
		{
			throw Diagnostics::Error("glTF animation contains a zero-length rotation keyframe.");
		}
		rotation = DirectX::XMQuaternionNormalize(rotation);
		if (previousRotation != nullptr
		    && DirectX::XMVectorGetX(DirectX::XMVector4Dot(DirectX::XMLoadFloat4(previousRotation), rotation)) < 0.0f)
		{
			rotation = DirectX::XMVectorNegate(rotation);
			if (cubicSpline)
			{
				DirectX::XMStoreFloat4(&keyframe.inTangent, DirectX::XMVectorNegate(DirectX::XMLoadFloat4(&keyframe.inTangent)));
				DirectX::XMStoreFloat4(&keyframe.outTangent, DirectX::XMVectorNegate(DirectX::XMLoadFloat4(&keyframe.outTangent)));
			}
		}
		DirectX::XMStoreFloat4(&keyframe.value, rotation);
	}

	static std::uint32_t ResolveMorphWeightCount(const cgltf_node& targetNode) noexcept
	{
		if (targetNode.mesh == nullptr || targetNode.mesh->primitives_count == 0)
		{
			return 0;
		}
		for (cgltf_size primitiveIndex = 0; primitiveIndex < targetNode.mesh->primitives_count; ++primitiveIndex)
		{
			if (targetNode.mesh->primitives[primitiveIndex].targets_count != 4u)
			{
				return 0;
			}
		}
		return 4u;
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

	static std::pair<std::uint32_t, std::uint32_t> FindSkeletonJointForNode(
	    const SourceImportOutput& output,
	    std::uint32_t sourceNodeIndex) noexcept
	{
		for (std::size_t skeletonIndex = 0; skeletonIndex < output.scene.skeletons.size(); ++skeletonIndex)
		{
			const ImportedSkeleton& skeleton = output.scene.skeletons[skeletonIndex];
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

	static ImportedSkeletonIndex FindSkeletonForSkin(
	    const SourceImportOutput& output,
	    const cgltf_data& data,
	    const cgltf_skin* skin) noexcept
	{
		if (skin == nullptr)
		{
			return kInvalidImportedSkeletonIndex;
		}
		const cgltf_size sourceSkinIndex = cgltf_skin_index(&data, skin);
		if (sourceSkinIndex >= data.skins_count)
		{
			return kInvalidImportedSkeletonIndex;
		}
		for (std::size_t skeletonIndex = 0; skeletonIndex < output.scene.skeletons.size(); ++skeletonIndex)
		{
			if (output.scene.skeletons[skeletonIndex].sourceSkinIndex == sourceSkinIndex)
			{
				return static_cast<ImportedSkeletonIndex>(skeletonIndex);
			}
		}
		return kInvalidImportedSkeletonIndex;
	}

	static ImportedAnimationSampler ImportSampler(
	    const cgltf_animation_sampler& sampler,
	    ImportedAnimationTargetPath targetPath,
	    std::uint32_t morphWeightCount,
	    float& inOutClipDurationSeconds)
	{
		ImportedAnimationSampler importedSampler;
		importedSampler.interpolation = ToImportedInterpolation(sampler.interpolation);
		const cgltf_accessor* input = sampler.input;
		const cgltf_accessor* output = sampler.output;
		if (input == nullptr || output == nullptr || input->count == 0 || input->type != cgltf_type_scalar
		    || input->component_type != cgltf_component_type_r_32f || output->component_type != cgltf_component_type_r_32f)
		{
			throw Diagnostics::Error("glTF animation sampler has incompatible input or output accessors.");
		}

		const std::uint32_t componentCount = GetValueComponentCount(targetPath);
		const bool morphWeights = targetPath == ImportedAnimationTargetPath::Weights;
		if ((morphWeights && morphWeightCount != 4u) || (!morphWeights && componentCount == 0u))
		{
			throw Diagnostics::Error("glTF animation sampler has an incompatible target value shape.");
		}
		const cgltf_type expectedOutputType = morphWeights ? cgltf_type_scalar : componentCount == 3u ? cgltf_type_vec3 : cgltf_type_vec4;
		if (output->type != expectedOutputType)
		{
			throw Diagnostics::Error("glTF animation sampler output type differs from its target path.");
		}
		const bool cubicSpline = importedSampler.interpolation == ImportedAnimationInterpolation::CubicSpline;
		const std::size_t outputElementsPerKeyframe = (cubicSpline ? 3u : 1u) * (morphWeights ? morphWeightCount : 1u);
		if (input->count > (std::numeric_limits<std::size_t>::max)() / outputElementsPerKeyframe)
		{
			throw Diagnostics::Error("glTF animation sampler keyframe count exceeds the engine range.");
		}
		const std::size_t expectedOutputCount = input->count * outputElementsPerKeyframe;
		if (output->count != expectedOutputCount)
		{
			throw Diagnostics::Error("glTF animation sampler input and output counts do not agree.");
		}
		importedSampler.keyframes.reserve(input->count);
		float previousTime = -1.0f;
		DirectX::XMFLOAT4 previousRotation{};
		bool hasPreviousRotation = false;
		for (cgltf_size keyframeIndex = 0; keyframeIndex < input->count; ++keyframeIndex)
		{
			ImportedAnimationKeyframe keyframe;
			keyframe.timeSeconds = ReadInputTime(input, keyframeIndex);
			if (keyframe.timeSeconds < 0.0f || keyframe.timeSeconds <= previousTime)
			{
				throw Diagnostics::Error(std::format("glTF animation keyframe {} is not strictly time ordered.", keyframeIndex));
			}

			const std::size_t outputBaseIndex = static_cast<std::size_t>(keyframeIndex) * outputElementsPerKeyframe;
			if (morphWeights)
			{
				if (cubicSpline)
				{
					keyframe.inTangent = ReadWeightOutputValue(output, outputBaseIndex);
					keyframe.value = ReadWeightOutputValue(output, outputBaseIndex + 4u);
					keyframe.outTangent = ReadWeightOutputValue(output, outputBaseIndex + 8u);
				}
				else
				{
					keyframe.value = ReadWeightOutputValue(output, outputBaseIndex);
				}
			}
			else
			{
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
			}

			NormalizeKeyframe(targetPath, cubicSpline, hasPreviousRotation ? &previousRotation : nullptr, keyframe);
			if (targetPath == ImportedAnimationTargetPath::Rotation)
			{
				previousRotation = keyframe.value;
				hasPreviousRotation = true;
			}
			inOutClipDurationSeconds = (std::max) (inOutClipDurationSeconds, keyframe.timeSeconds);
			importedSampler.keyframes.push_back(keyframe);
			previousTime = keyframe.timeSeconds;
		}

		return importedSampler;
	}

	static void ImportChannel(
	    const cgltf_data& data,
	    const cgltf_animation& animation,
	    const cgltf_animation_channel& channel,
	    std::size_t channelIndex,
	    const SourceImportOutput& output,
	    std::vector<ImportedAnimationTargetPath>& samplerTargetPaths,
	    ImportedAnimationClip& clip)
	{
		const ImportedAnimationTargetPath targetPath = ToImportedTargetPath(channel.target_path);
		const std::uint32_t samplerIndex = FindSamplerIndex(animation, channel.sampler);
		if (targetPath == ImportedAnimationTargetPath::Unknown || samplerIndex >= clip.samplers.size() || channel.target_node == nullptr)
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} has an invalid target or sampler.", channelIndex));
		}
		if (samplerTargetPaths[samplerIndex] != ImportedAnimationTargetPath::Unknown && samplerTargetPaths[samplerIndex] != targetPath)
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} reuses a sampler for another target path.", channelIndex));
		}
		samplerTargetPaths[samplerIndex] = targetPath;

		const std::uint32_t morphWeightCount =
		    targetPath == ImportedAnimationTargetPath::Weights ? ResolveMorphWeightCount(*channel.target_node) : 0u;
		if (targetPath == ImportedAnimationTargetPath::Weights && morphWeightCount != 4u)
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} does not target exactly four morph weights.", channelIndex));
		}
		if (clip.samplers[samplerIndex].keyframes.empty())
		{
			clip.samplers[samplerIndex] = ImportSampler(*channel.sampler, targetPath, morphWeightCount, clip.durationSeconds);
		}

		const std::uint32_t targetNodeIndex = static_cast<std::uint32_t>(cgltf_node_index(&data, channel.target_node));
		const auto [jointSkeletonIndex, targetJointIndex] = FindSkeletonJointForNode(output, targetNodeIndex);
		const ImportedSkeletonIndex targetSkeletonIndex = targetPath == ImportedAnimationTargetPath::Weights
		    ? FindSkeletonForSkin(output, data, channel.target_node->skin)
		    : jointSkeletonIndex;
		if (targetSkeletonIndex == kInvalidImportedSkeletonIndex
		    || (targetPath != ImportedAnimationTargetPath::Weights && targetJointIndex == (std::numeric_limits<std::uint32_t>::max)()))
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} is not owned by an imported skeleton.", channelIndex));
		}
		if (clip.targetSkeletonIndex != kInvalidImportedSkeletonIndex && clip.targetSkeletonIndex != targetSkeletonIndex)
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} targets another skeleton.", channelIndex));
		}
		clip.targetSkeletonIndex = targetSkeletonIndex;

		if (std::any_of(
		        clip.channels.begin(),
		        clip.channels.end(),
		        [targetPath, targetNodeIndex](const ImportedAnimationChannel& importedChannel)
		        { return importedChannel.targetPath == targetPath && importedChannel.targetNodeIndex == targetNodeIndex; }))
		{
			throw Diagnostics::Error(std::format("glTF animation channel {} duplicates an existing target path.", channelIndex));
		}

		clip.channels.push_back(
		    ImportedAnimationChannel{
		        .targetPath = targetPath,
		        .targetNodeIndex = targetNodeIndex,
		        .targetJointIndex = targetJointIndex,
		        .samplerIndex = samplerIndex});
	}
};

void GltfAnimationImporter::ImportAnimations(const cgltf_data* data, SourceImportOutput& output)
{
	if (data == nullptr)
	{
		throw Diagnostics::Error("glTF animation import has no parsed scene.");
	}
	if (data->animations_count == 0)
	{
		return;
	}
	// Sparkle animation clips are skeleton-owned. Preserve static scene import for
	// glTF assets that contain only node or morph animation until a node-animation
	// runtime exists; skeletal assets continue through strict channel validation.
	if (output.scene.skeletons.empty())
	{
		return;
	}

	output.scene.animations.reserve(data->animations_count);
	for (cgltf_size animationIndex = 0; animationIndex < data->animations_count; ++animationIndex)
	{
		const cgltf_animation& animation = data->animations[animationIndex];
		ImportedAnimationClip clip;
		clip.name = animation.name != nullptr ? animation.name : "";
		clip.sourceAnimationIndex = static_cast<std::uint32_t>(animationIndex);
		clip.samplers.resize(animation.samplers_count);
		clip.channels.reserve(animation.channels_count);
		std::vector<ImportedAnimationTargetPath> samplerTargetPaths(animation.samplers_count, ImportedAnimationTargetPath::Unknown);

		for (cgltf_size channelIndex = 0; channelIndex < animation.channels_count; ++channelIndex)
		{
			GltfAnimationTranslation::ImportChannel(
			    *data,
			    animation,
			    animation.channels[channelIndex],
			    channelIndex,
			    output,
			    samplerTargetPaths,
			    clip);
		}

		if (!clip.IsValid())
		{
			throw Diagnostics::Error(std::format("glTF animation {} has no complete channel set.", animationIndex));
		}
		output.scene.animations.push_back(std::move(clip));
	}
}
