#pragma once

#include "Animation/AnimationEvaluationTypes.h"
#include "GameFramework/Public/Scene/Animations/AnimationOutput.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/Resources/AnimationClipResourceStore.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

namespace ECS
{
	class EntityRegistry;
	class MorphWeightStorage;

	class AnimationOutputStorage final
	{
	  public:
		struct PoseWorkSlot final
		{
			EntityId Entity;
			AnimationResourceHandle Clip;
			SkeletonResourceHandle Skeleton;
			std::uint64_t SourceInstanceId = 0;
			std::uint32_t PoseOutputIndex = (std::numeric_limits<std::uint32_t>::max)();
			std::vector<AnimationJointTransform> LocalTransforms;
			std::vector<DirectX::XMFLOAT4X4> ModelSpaceTransforms;
		};

		struct MorphSampleSlot final
		{
			EntityId AnimationEntity;
			AnimationResourceHandle Clip;
			std::uint64_t SourceInstanceId = 0;
			std::uint32_t ChannelIndex = 0;
			std::uint32_t OutputIndex = 0;
		};

		struct MorphTargetBinding final
		{
			std::uint32_t SampleIndex = 0;
			EntityId TargetEntity;
			AnimationOutputSlotHandle TargetWeights;
		};

		bool Prepare(
		    EntityRegistry& registry,
		    const AnimationClipResourceStore& clips,
		    const SkeletonResourceStore& skeletons,
		    MorphWeightStorage& morphWeights,
		    std::uint32_t targetGeneration);

		PoseWorkSlot* FindPoseWork(EntityId entity) noexcept;
		std::span<PoseWorkSlot> GetPoseWork() noexcept { return m_poseWork; }
		std::span<MorphSampleSlot> GetMorphSamples() noexcept { return m_morphSamples; }
		std::span<const EntityId> GetMorphEntities() const noexcept { return m_morphEntities; }
		std::span<const MorphTargetBinding> GetMorphBindings() const noexcept { return m_morphBindings; }
		AnimationOutput& GetMutableOutput() noexcept { return m_output; }
		const AnimationOutput& GetOutput() const noexcept { return m_output; }
		std::uint32_t GetTargetGeneration() const noexcept { return m_targetGeneration; }

	  private:
		struct EntityWorkIndex final
		{
			EntityId Entity;
			std::uint32_t Index = (std::numeric_limits<std::uint32_t>::max)();
		};

		std::vector<PoseWorkSlot> m_poseWork;
		std::vector<MorphSampleSlot> m_morphSamples;
		std::vector<EntityId> m_morphEntities;
		std::vector<MorphTargetBinding> m_morphBindings;
		std::vector<EntityWorkIndex> m_workIndexByEntitySlot;
		AnimationOutput m_output;
		std::uint64_t m_structureVersion = 0;
		std::uint32_t m_targetGeneration = 0;
	};
}
