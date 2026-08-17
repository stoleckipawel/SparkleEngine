#include "Animation/AnimationPoseEvaluator.h"
#include "Animation/AnimationSampler.h"
#include "Animation/SkinningMatrixEvaluator.h"
#include "GameFramework/Public/Scene/Animations/SkeletonTransformContract.h"

#include <DirectXMath.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace AnimationContractTests
{
	constexpr float kEpsilon = 1.0e-4f;

	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	bool Near(float lhs, float rhs, float epsilon = kEpsilon) noexcept
	{
		return std::abs(lhs - rhs) <= epsilon;
	}

	DirectX::XMFLOAT3 TranslationOf(const DirectX::XMFLOAT4X4& matrix) noexcept
	{
		return {matrix._41, matrix._42, matrix._43};
	}

	bool MatrixIsIdentity(const DirectX::XMFLOAT4X4& matrix, float epsilon = kEpsilon) noexcept
	{
		DirectX::XMFLOAT4X4 identity;
		DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
		const float* actual = &matrix._11;
		const float* expected = &identity._11;
		for (std::size_t index = 0; index < 16u; ++index)
		{
			if (!Near(actual[index], expected[index], epsilon))
			{
				return false;
			}
		}
		return true;
	}

	AnimationChannel MakeChannel(Assets::CookedAnimationInterpolation interpolation, std::uint32_t keyframeCount) noexcept
	{
		return AnimationChannel{
		    .targetPath = Assets::CookedAnimationTargetPath::Translation,
		    .interpolation = interpolation,
		    .targetJointIndex = 0u,
		    .firstKeyframe = 0u,
		    .keyframeCount = keyframeCount};
	}

	void StepInterpolationSelectsTheLastReachedKey()
	{
		AnimationClipResource clip;
		clip.keyframes = {
		    AnimationKeyframe{.timeSeconds = 0.0f, .value = {1.0f, 0.0f, 0.0f, 0.0f}},
		    AnimationKeyframe{.timeSeconds = 1.0f, .value = {9.0f, 0.0f, 0.0f, 0.0f}}};
		const AnimationChannel channel = MakeChannel(Assets::CookedAnimationInterpolation::Step, 2u);

		Require(
		    Near(DirectX::XMVectorGetX(AnimationSampler::SampleVectorChannel(clip, channel, 0.999f)), 1.0f),
		    "Step animation advanced before the next key time.");
		Require(
		    Near(DirectX::XMVectorGetX(AnimationSampler::SampleVectorChannel(clip, channel, 1.0f)), 9.0f),
		    "Step animation did not advance at the next key time.");
	}

	void CubicSplineUsesDurationScaledTangents()
	{
		AnimationClipResource clip;
		clip.keyframes = {
		    AnimationKeyframe{.timeSeconds = 0.0f, .value = {0.0f, 0.0f, 0.0f, 0.0f}, .outTangent = {4.0f, 0.0f, 0.0f, 0.0f}},
		    AnimationKeyframe{.timeSeconds = 2.0f, .value = {2.0f, 0.0f, 0.0f, 0.0f}, .inTangent = {0.0f, 0.0f, 0.0f, 0.0f}}};
		const AnimationChannel channel = MakeChannel(Assets::CookedAnimationInterpolation::CubicSpline, 2u);

		Require(
		    Near(DirectX::XMVectorGetX(AnimationSampler::SampleVectorChannel(clip, channel, 1.0f)), 2.0f),
		    "Cubic animation did not apply glTF's duration-scaled Hermite tangents.");
	}

	void PoseOrderAndSkinReferencePreserveTheBindPose()
	{
		SkeletonResource skeleton;
		skeleton.joints.resize(2u);
		SkeletonJoint& child = skeleton.joints[0];
		SkeletonJoint& root = skeleton.joints[1];
		child.parentJointIndex = 1u;
		root.parentJointIndex = (std::numeric_limits<std::uint32_t>::max)();
		DirectX::XMStoreFloat4x4(&root.parentSpaceTransform, DirectX::XMMatrixTranslation(0.0f, 2.0f, 0.0f));
		DirectX::XMStoreFloat4x4(&root.bindModelTransform, DirectX::XMMatrixTranslation(1.0f, 2.0f, 0.0f));
		DirectX::XMStoreFloat4x4(&child.parentSpaceTransform, DirectX::XMMatrixTranslation(0.0f, 4.0f, 0.0f));
		DirectX::XMStoreFloat4x4(&child.bindModelTransform, DirectX::XMMatrixTranslation(1.0f, 6.0f, 3.0f));
		DirectX::XMStoreFloat4x4(
		    &root.inverseBindMatrix,
		    DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&root.bindModelTransform)));
		DirectX::XMStoreFloat4x4(
		    &child.inverseBindMatrix,
		    DirectX::XMMatrixInverse(nullptr, DirectX::XMLoadFloat4x4(&child.bindModelTransform)));

		const std::array<ECS::AnimationJointTransform, 2> bindLocalTransforms = {
		    ECS::AnimationJointTransform{.Translation = {0.0f, 0.0f, 3.0f}},
		    ECS::AnimationJointTransform{.Translation = {1.0f, 0.0f, 0.0f}}};
		const std::array<std::uint32_t, 2> evaluationOrder = {1u, 0u};
		const ECS::SkeletonEvaluationData evaluation{
		    .Resource = &skeleton,
		    .BindLocalTransforms = bindLocalTransforms,
		    .EvaluationOrder = evaluationOrder};
		AnimationClipResource clip;
		std::array<ECS::AnimationJointTransform, 2> localTransforms;
		std::array<DirectX::XMFLOAT4X4, 2> modelTransforms;
		Require(
		    AnimationPoseEvaluator::Evaluate(clip, evaluation, 0.0f, localTransforms, modelTransforms),
		    "Bind-pose evaluation was rejected.");

		const DirectX::XMFLOAT3 rootTranslation = TranslationOf(modelTransforms[1]);
		const DirectX::XMFLOAT3 childTranslation = TranslationOf(modelTransforms[0]);
		Require(
		    Near(rootTranslation.x, 1.0f) && Near(rootTranslation.y, 2.0f) && Near(rootTranslation.z, 0.0f),
		    "Root JointParentSpace was not applied in evaluation order.");
		Require(
		    Near(childTranslation.x, 1.0f) && Near(childTranslation.y, 6.0f) && Near(childTranslation.z, 3.0f),
		    "Child Local * JointParentSpace * ParentModel composition is incorrect.");

		std::array<DirectX::XMFLOAT4X4, 2> skinningMatrices;
		Require(
		    SkinningMatrixEvaluator::Evaluate(evaluation, modelTransforms, skinningMatrices),
		    "Bind-pose skinning evaluation was rejected.");
		Require(
		    MatrixIsIdentity(skinningMatrices[0]) && MatrixIsIdentity(skinningMatrices[1]),
		    "InverseBind * BindModel did not preserve the bind pose.");
	}

	void SkeletonEvaluationOrderIsCanonicalAndRejectsCycles()
	{
		std::vector<SkeletonJoint> joints(3u);
		joints[0].parentJointIndex = 1u;
		joints[1].parentJointIndex = 2u;
		joints[2].parentJointIndex = (std::numeric_limits<std::uint32_t>::max)();
		std::vector<std::uint32_t> evaluationOrder;
		Require(
		    SkeletonTransformContract::BuildEvaluationOrder(joints, evaluationOrder),
		    "Valid child-before-parent hierarchy was rejected.");
		Require(
		    evaluationOrder == std::vector<std::uint32_t>{2u, 1u, 0u},
		    "Skeleton evaluation order did not place every parent before its children.");

		joints[2].parentJointIndex = 0u;
		Require(!SkeletonTransformContract::BuildEvaluationOrder(joints, evaluationOrder), "Cyclic skeleton hierarchy was accepted.");
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main()
{
	int failureCount = 0;
	failureCount += AnimationContractTests::Run("step key selection", AnimationContractTests::StepInterpolationSelectsTheLastReachedKey);
	failureCount +=
	    AnimationContractTests::Run("cubic duration-scaled tangents", AnimationContractTests::CubicSplineUsesDurationScaledTangents);
	failureCount += AnimationContractTests::Run(
	    "pose order and skin-reference bind invariant",
	    AnimationContractTests::PoseOrderAndSkinReferencePreserveTheBindPose);
	failureCount += AnimationContractTests::Run(
	    "skeleton evaluation order",
	    AnimationContractTests::SkeletonEvaluationOrderIsCanonicalAndRejectsCycles);
	return failureCount == 0 ? 0 : 1;
}
