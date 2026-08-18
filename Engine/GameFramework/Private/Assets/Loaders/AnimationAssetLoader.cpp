#include "PCH.h"

#include "Assets/Loaders/AnimationAssetLoader.h"

#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Loaders/CookedAssetByteReader.h"
#include "Assets/Loaders/CookedAssetLoaderDiagnostics.h"
#include "Core/Public/Strings/StringUtils.h"

#include <DirectXMath.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>

namespace AnimationAssetLoaderValidation
{
	bool IsKnownTargetPath(Assets::CookedAnimationTargetPath targetPath) noexcept
	{
		switch (targetPath)
		{
			case Assets::CookedAnimationTargetPath::Translation:
			case Assets::CookedAnimationTargetPath::Rotation:
			case Assets::CookedAnimationTargetPath::Scale:
			case Assets::CookedAnimationTargetPath::Weights:
				return true;
			case Assets::CookedAnimationTargetPath::Unknown:
			default:
				return false;
		}
	}

	bool IsKnownInterpolation(Assets::CookedAnimationInterpolation interpolation) noexcept
	{
		switch (interpolation)
		{
			case Assets::CookedAnimationInterpolation::Linear:
			case Assets::CookedAnimationInterpolation::Step:
			case Assets::CookedAnimationInterpolation::CubicSpline:
				return true;
			default:
				return false;
		}
	}

	bool IsFinite(const DirectX::XMFLOAT4& value) noexcept
	{
		return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) && std::isfinite(value.w);
	}
}

namespace Assets
{
	LoadedAnimationAsset AnimationAssetLoader::Decode(const std::filesystem::path& path, std::span<const std::uint8_t> bytes) const
	{
		const CookedAssetLoaderDiagnostics diagnostics(path, "CookedAnimationAsset");

		CookedAssetByteReader reader(bytes);
		LoadedAnimationAsset animationAsset;
		animationAsset.header = reader.Read<CookedAnimationAssetHeader>();

		if (!animationAsset.header.fileHeader.HasMagic(kCookedAnimationAssetMagic)
		    || animationAsset.header.channelStride != sizeof(CookedAnimationChannelRecord)
		    || animationAsset.header.keyframeStride != sizeof(CookedAnimationKeyframeRecord)
		    || !Strings::IsNullTerminated(std::span(animationAsset.header.name)))
		{
			throw diagnostics.MakeError(
			    "header",
			    "animation magic and current channel/keyframe strides",
			    "Invalid cooked animation asset header; recook the asset");
		}

		animationAsset.channels = reader.ReadArray<CookedAnimationChannelRecord>(animationAsset.header.channelCount);
		animationAsset.keyframes = reader.ReadArray<CookedAnimationKeyframeRecord>(animationAsset.header.keyframeCount);
		if (!std::isfinite(animationAsset.header.durationSeconds) || animationAsset.header.durationSeconds < 0.0f)
		{
			throw diagnostics.MakeError("payload", "a finite non-negative clip duration", "Cooked animation asset has an invalid duration");
		}

		for (std::size_t channelIndex = 0; channelIndex < animationAsset.channels.size(); ++channelIndex)
		{
			const CookedAnimationChannelRecord& channel = animationAsset.channels[channelIndex];
			if (!AnimationAssetLoaderValidation::IsKnownTargetPath(channel.targetPath)
			    || !AnimationAssetLoaderValidation::IsKnownInterpolation(channel.interpolation)
			    || channel.targetNodeIndex == (std::numeric_limits<std::uint32_t>::max)() || channel.keyframeCount == 0u
			    || channel.firstKeyframe > animationAsset.keyframes.size()
			    || channel.keyframeCount > animationAsset.keyframes.size() - channel.firstKeyframe)
			{
				throw diagnostics.MakeError(
				    "payload",
				    "known channel semantics and an in-range non-empty keyframe span",
				    std::format("Cooked animation channel {} is invalid", channelIndex));
			}

			float previousTime = -1.0f;
			DirectX::XMFLOAT4 previousRotation{};
			bool hasPreviousRotation = false;
			for (std::uint32_t keyframeOffset = 0; keyframeOffset < channel.keyframeCount; ++keyframeOffset)
			{
				const CookedAnimationKeyframeRecord& keyframe = animationAsset.keyframes[channel.firstKeyframe + keyframeOffset];
				const bool cubicSpline = channel.interpolation == CookedAnimationInterpolation::CubicSpline;
				if (!std::isfinite(keyframe.timeSeconds) || keyframe.timeSeconds < 0.0f || keyframe.timeSeconds <= previousTime
				    || keyframe.timeSeconds > animationAsset.header.durationSeconds
				    || !AnimationAssetLoaderValidation::IsFinite(keyframe.value)
				    || (cubicSpline
				        && (!AnimationAssetLoaderValidation::IsFinite(keyframe.inTangent)
				            || !AnimationAssetLoaderValidation::IsFinite(keyframe.outTangent))))
				{
					throw diagnostics.MakeError(
					    "payload",
					    "finite strictly ordered keyframes within the clip duration",
					    std::format("Cooked animation channel {} keyframe {} is invalid", channelIndex, keyframeOffset));
				}

				if (channel.targetPath == CookedAnimationTargetPath::Rotation)
				{
					const DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&keyframe.value);
					const float lengthSquared = DirectX::XMVectorGetX(DirectX::XMVector4LengthSq(rotation));
					if (!std::isfinite(lengthSquared) || std::abs(lengthSquared - 1.0f) > 1.0e-3f
					    || (hasPreviousRotation
					        && DirectX::XMVectorGetX(DirectX::XMVector4Dot(DirectX::XMLoadFloat4(&previousRotation), rotation)) < -1.0e-5f))
					{
						throw diagnostics.MakeError(
						    "payload",
						    "normalized sign-continuous rotation keys",
						    std::format("Cooked animation channel {} keyframe {} has an invalid rotation", channelIndex, keyframeOffset));
					}
					previousRotation = keyframe.value;
					hasPreviousRotation = true;
				}
				previousTime = keyframe.timeSeconds;
			}
		}

		if (reader.GetRemainingByteCount() != 0)
		{
			throw diagnostics.MakeError(
			    "payload",
			    "no trailing bytes after declared animation records",
			    "Cooked animation asset contains unexpected trailing bytes");
		}

		return animationAsset;
	}
}
