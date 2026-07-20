#include "Animation/AnimationEvaluationTypes.h"
#include "Animation/AnimationPoseEvaluator.h"
#include "Animation/MorphWeightEvaluator.h"
#include "Animation/SkinningMatrixEvaluator.h"
#include "Core/Public/FileSystemUtils.h"
#include "GameFramework/Public/Level/LevelManager.h"
#include "GameFramework/Public/Scene/Animations/AnimationClipResource.h"
#include "GameFramework/Public/Scene/Camera/CameraInputIntent.h"
#include "GameFramework/Public/Scene/Skeletons/SkeletonResource.h"
#include "GameFramework/Public/World/GameWorld.h"
#include "Tasks/Public/TaskExecutor.h"
#include "Tasks/Public/TaskScope.h"
#include "World/ECS/EntityRegistry.h"
#include "World/ECS/Query.h"
#include "World/Systems/GameSystemGraph.h"

#include <DirectXMath.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <memory>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace
{
	using namespace ECS;
	using Clock = std::chrono::steady_clock;

	struct ValidationComponent final
	{
		std::uint32_t Value = 0;
	};

	struct WorldResult final
	{
		CameraSnapshot Camera;
		std::vector<DirectX::XMFLOAT4X4> MeshTransforms;
		AnimationOutput Animations;
	};

	struct AnimationWorkSlot final
	{
		std::vector<AnimationJointTransform> Local;
		std::vector<DirectX::XMFLOAT4X4> Model;
		std::vector<DirectX::XMFLOAT4X4> Skinning;
	};

	bool g_succeeded = true;

	void Check(bool condition, std::string_view label)
	{
		if (condition)
			return;
		g_succeeded = false;
		std::cerr << "FAIL: " << label << '\n';
	}

	GameSystemExecutionPolicy Ranges(
	    std::uint32_t grain = 8,
	    std::uint32_t threshold = 16,
	    std::uint32_t partitions = 8)
	{
		return {GameSystemExecutionMode::ParallelRanges, {grain, threshold, partitions}};
	}

	GameSystemDesc MakeSystem(
	    std::string_view stableName,
	    GameSystemPhase phase = GameSystemPhase::Simulation)
	{
		GameSystemDesc result{MakeGameSystemId(stableName), std::string(stableName), phase};
		result.Execution = Ranges();
		return result;
	}

	void ValidateCompilerRejections()
	{
		{
			GameSystemGraph graph;
			graph.Add(MakeSystem("Validation.Duplicate"));
			graph.Add(MakeSystem("Validation.Duplicate"));
			Check(graph.Compile().GetError().Code == GameSystemGraphErrorCode::DuplicateSystem, "duplicate system rejection");
		}
		{
			GameSystemDesc first = MakeSystem("Validation.CycleA");
			GameSystemDesc second = MakeSystem("Validation.CycleB");
			first.Prerequisites = {second.Id};
			second.Prerequisites = {first.Id};
			GameSystemGraph graph;
			graph.Add(std::move(first));
			graph.Add(std::move(second));
			Check(graph.Compile().GetError().Code == GameSystemGraphErrorCode::Cycle, "cycle rejection");
		}
		{
			GameSystemDesc first = MakeSystem("Validation.HazardA");
			GameSystemDesc second = MakeSystem("Validation.HazardB");
			first.Resources = {{GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Write}};
			second.Resources = {{GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Read}};
			GameSystemGraph graph;
			graph.Add(std::move(first));
			graph.Add(std::move(second));
			Check(graph.Compile().GetError().Code == GameSystemGraphErrorCode::AmbiguousHazard, "unordered access rejection");
		}
		{
			GameSystemDesc system = MakeSystem("Validation.ConflictingAccess");
			const RuntimeComponentTypeId type = ComponentTypeRegistry::GetTypeId<ValidationComponent>();
			system.Components = {
			    {type, ComponentAccessMode::Read},
			    {type, ComponentAccessMode::Write}};
			GameSystemGraph graph;
			graph.Add(std::move(system));
			Check(
			    graph.Compile().GetError().Code == GameSystemGraphErrorCode::ConflictingAccessDeclaration,
			    "conflicting component access rejection");
		}
		{
			GameSystemDesc system = MakeSystem("Validation.UnavailableResource");
			system.Resources = {{GameSystemResourceDomain::ExtractionOutput, GameSystemAccessMode::Read}};
			GameSystemGraph graph;
			graph.Add(std::move(system));
			Check(
			    graph.Compile().GetError().Code == GameSystemGraphErrorCode::UnavailablePhaseResource,
			    "phase resource rejection");
		}
	}

	void ValidateFrozenStructureAndRetainedView()
	{
		EntityRegistry registry;
		const EntityId entity = registry.Create();
		Check(entity.IsValid() && registry.Add(entity, ValidationComponent{7}), "validation fixture creation");

		using ValidationQuery = Query<Write<ValidationComponent>>;
		std::unique_ptr<ValidationQuery> retained;
		{
			StructureFrozenEpoch epoch = registry.FreezeStructure();
			retained = std::make_unique<ValidationQuery>(registry, epoch);
			Check(!registry.Create().IsValid(), "worker structural create rejection");
			Check(!registry.Destroy(entity), "worker structural destroy rejection");
			Check(!registry.Add(entity, ValidationComponent{9}), "worker structural add rejection");
			Check(retained->PrepareWriteTraversal(), "query write preparation");
			Check(
			    retained->ForEachRange(
			                0,
			                1,
			                [](std::size_t, EntityId, ValidationComponent& component) { component.Value = 11; })
			        .Succeeded(),
			    "exclusive range mutation");
		}
		Check(
		    retained->ForEachRange(0, 1, [](std::size_t, EntityId, ValidationComponent&) {}).Status ==
		        QueryIterationStatus::InvalidEpoch,
		    "retained query rejection");
	}

	std::vector<std::uint64_t> ExecuteDeterministicRanges(std::uint32_t workerCount, std::uint32_t seed)
	{
		constexpr std::uint32_t Count = 4096;
		std::vector<std::uint64_t> source(Count);
		std::vector<std::uint64_t> first(Count);
		std::vector<std::uint64_t> second(Count);
		for (std::uint32_t index = 0; index < Count; ++index)
			source[index] = index * 0x9e3779b97f4a7c15ull + seed;

		GameSystemDesc transform = MakeSystem("Validation.Transform");
		transform.Resources = {{GameSystemResourceDomain::MotionClock, GameSystemAccessMode::Read}};
		transform.Execution = Ranges(32, 64, 16);
		GameSystemDesc extraction = MakeSystem("Validation.Extraction", GameSystemPhase::Extraction);
		extraction.Resources = {
		    {GameSystemResourceDomain::ExtractionScratch, GameSystemAccessMode::Write},
		    {GameSystemResourceDomain::ExtractionOutput, GameSystemAccessMode::Write}};
		extraction.Prerequisites = {transform.Id};
		extraction.Execution = Ranges(32, 64, 16);

		GameSystemGraph graph;
		graph.Add(transform);
		graph.Add(extraction);
		CompiledGameSystemGraph compiled = graph.Compile();
		Check(compiled.IsValid(), "deterministic range graph compile");

		const auto jitter = [seed](std::uint32_t index)
		{
			if (((index * 17u + seed * 13u) & 31u) == 0u)
				std::this_thread::yield();
		};
		std::array<GameSystemExecutionBinding, 2> bindings{{
		    {transform.Id,
		     [] { return Count; },
		     [&](std::uint32_t begin, std::uint32_t end)
		     {
			     for (std::uint32_t index = begin; index < end; ++index)
			     {
				     jitter(index);
				     first[index] = (source[index] ^ 0xd6e8feb86659fd93ull) * 0x94d049bb133111ebull;
			     }
			     return true;
		     }},
		    {extraction.Id,
		     [] { return Count; },
		     [&](std::uint32_t begin, std::uint32_t end)
		     {
			     for (std::uint32_t index = begin; index < end; ++index)
			     {
				     jitter(Count - index);
				     second[index] = first[index] + index;
			     }
			     return true;
		     }}}};

		TaskExecutorConfig config{};
		if (workerCount != 0)
			config.FrameCriticalWorkerCount = workerCount;
		TaskExecutor executor(config);
		GameSystemGraphError error;
		Check(compiled.Execute(executor, bindings, error), "deterministic range execution");
		return second;
	}

	void ValidateWorkerMatrix()
	{
		for (std::uint32_t seed = 1; seed <= 12; ++seed)
		{
			const std::vector<std::uint64_t> serial = ExecuteDeterministicRanges(0, seed);
			for (std::uint32_t workers : {1u, 2u, 4u})
				Check(ExecuteDeterministicRanges(workers, seed) == serial, "serial/1/2/N deterministic output");
		}
	}

	bool NearlyEqual(float lhs, float rhs, float tolerance = 1.0e-5f)
	{
		return std::abs(lhs - rhs) <= tolerance;
	}

	bool MatrixEqual(const DirectX::XMFLOAT4X4& lhs, const DirectX::XMFLOAT4X4& rhs)
	{
		for (std::size_t row = 0; row < 4; ++row)
		{
			for (std::size_t column = 0; column < 4; ++column)
			{
				if (!NearlyEqual(lhs.m[row][column], rhs.m[row][column]))
					return false;
			}
		}
		return true;
	}

	WorldResult ExecuteWorldSequence(std::uint32_t workerCount)
	{
		TaskExecutorConfig config{};
		if (workerCount != 0)
		{
			config.FrameCriticalWorkerCount = workerCount;
			config.BackgroundWorkerCount = workerCount;
			config.BlockingIoWorkerCount = 1;
		}
		TaskExecutor executor(config);
		TaskScope scope(TaskScopeDesc{TaskScopeKind::Application, "Prompt10.Validation"});
		GameWorld world(executor);
		{
			LevelManager levels(world, executor, scope);
			levels.RequestLevelChange("SponzaPtlas");
			const auto loadDeadline = Clock::now() + std::chrono::seconds(30);
			while (!levels.HasActiveLevel() && Clock::now() < loadDeadline)
			{
				levels.ProcessPendingLevelChange();
				if (levels.IsLevelChangeInProgress())
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				else if (!levels.GetLastLoadDiagnostic().empty())
					break;
			}
			Check(levels.HasActiveLevel(), "SponzaPtlas generation load");
			Check(levels.GetLastLoadDiagnostic().empty(), "SponzaPtlas generation diagnostic");
			if (!levels.HasActiveLevel())
				std::cerr << "load diagnostic: " << levels.GetLastLoadDiagnostic() << '\n';
			else
			{
				world.EnableOscillatingMeshMotion();
				for (std::uint32_t frame = 0; frame < 120; ++frame)
				{
					CameraInputIntent intent;
					intent.ForwardAxis = frame % 3 == 0 ? 1.0f : -0.25f;
					intent.RightAxis = frame % 5 == 0 ? 0.5f : 0.0f;
					intent.UpAxis = frame % 11 == 0 ? 0.25f : 0.0f;
					intent.LookDeltaX = static_cast<float>(static_cast<int>(frame % 7) - 3) * 0.015f;
					intent.LookDeltaY = static_cast<float>(static_cast<int>(frame % 5) - 2) * 0.01f;
					intent.SpeedStepCount = frame == 24 ? 1.0f : 0.0f;
					intent.AspectRatio = 16.0f / 9.0f;
					intent.HasAspectRatio = true;
					intent.Sprint = frame >= 40 && frame < 60;
					world.PublishCameraInputIntent(intent);
					world.Update(1.0f / 60.0f);
				}
			}
		}

		GameWorldSnapshot snapshot = world.CaptureSnapshot();
		WorldResult result;
		result.Camera = snapshot.camera;
		result.Animations = std::move(snapshot.animations);
		result.MeshTransforms.reserve(snapshot.meshes.meshInstances.size());
		for (const MeshInstanceSnapshot& instance : snapshot.meshes.meshInstances)
			result.MeshTransforms.push_back(instance.worldMatrix);
		scope.Cancel();
		executor.Shutdown(TaskExecutorShutdownMode::Drain);
		Check(scope.JoinFor(std::chrono::seconds(1)), "world validation scope settlement");
		return result;
	}

	void ValidateWorldParity()
	{
		const WorldResult serial = ExecuteWorldSequence(0);
		for (std::uint32_t workers : {1u, 2u, 4u})
		{
			const WorldResult parallel = ExecuteWorldSequence(workers);
			Check(serial.MeshTransforms.size() == parallel.MeshTransforms.size(), "mesh extraction count parity");
			Check(serial.Animations.poses.size() == parallel.Animations.poses.size(), "pose count parity");
			Check(serial.Animations.morphWeights.size() == parallel.Animations.morphWeights.size(), "morph count parity");
			Check(
			    NearlyEqual(serial.Camera.position.x, parallel.Camera.position.x) &&
			        NearlyEqual(serial.Camera.position.y, parallel.Camera.position.y) &&
			        NearlyEqual(serial.Camera.position.z, parallel.Camera.position.z) &&
			        NearlyEqual(serial.Camera.direction.x, parallel.Camera.direction.x) &&
			        NearlyEqual(serial.Camera.direction.y, parallel.Camera.direction.y) &&
			        NearlyEqual(serial.Camera.direction.z, parallel.Camera.direction.z),
			    "camera navigation parity");
			for (std::size_t index = 0; index < (std::min)(serial.MeshTransforms.size(), parallel.MeshTransforms.size()); ++index)
				Check(MatrixEqual(serial.MeshTransforms[index], parallel.MeshTransforms[index]), "PTLAS motion/transform/extraction parity");
			for (std::size_t poseIndex = 0; poseIndex < (std::min)(serial.Animations.poses.size(), parallel.Animations.poses.size()); ++poseIndex)
			{
				const AnimationPoseOutput& lhs = serial.Animations.poses[poseIndex];
				const AnimationPoseOutput& rhs = parallel.Animations.poses[poseIndex];
				Check(
				    lhs.animationEntity == rhs.animationEntity && lhs.animationAssetId == rhs.animationAssetId &&
				        lhs.skeletonAssetId == rhs.skeletonAssetId && NearlyEqual(lhs.playbackTimeSeconds, rhs.playbackTimeSeconds) &&
				        lhs.skinningMatrices.size() == rhs.skinningMatrices.size(),
				    "animation pose identity parity");
				for (std::size_t joint = 0; joint < (std::min)(lhs.skinningMatrices.size(), rhs.skinningMatrices.size()); ++joint)
					Check(MatrixEqual(lhs.skinningMatrices[joint], rhs.skinningMatrices[joint]), "skinning matrix parity");
			}
		}
		std::cout << "world_meshes=" << serial.MeshTransforms.size() << " world_poses=" << serial.Animations.poses.size()
		          << " world_morph_outputs=" << serial.Animations.morphWeights.size() << '\n';
	}

	void ValidateMorphSampling()
	{
		AnimationClipResource clip;
		clip.durationSeconds = 1.0f;
		clip.channels.push_back(AnimationChannel{
		    .targetPath = Assets::CookedAnimationTargetPath::Weights,
		    .interpolation = Assets::CookedAnimationInterpolation::Linear,
		    .firstKeyframe = 0,
		    .keyframeCount = 2});
		clip.keyframes = {
		    AnimationKeyframe{.timeSeconds = 0.0f, .value = {0.0f, 0.2f, 0.4f, 0.6f}},
		    AnimationKeyframe{.timeSeconds = 1.0f, .value = {1.0f, 0.8f, 0.6f, 0.4f}}};
		std::array<float, 4> weights{};
		Check(MorphWeightEvaluator::Evaluate(clip, 0, 0.5f, weights), "morph sample evaluation");
		Check(
		    NearlyEqual(weights[0], 0.5f) && NearlyEqual(weights[1], 0.5f) && NearlyEqual(weights[2], 0.5f) &&
		        NearlyEqual(weights[3], 0.5f),
		    "morph interpolation parity");
	}

	void BuildAnimationFixture(
	    std::uint32_t jointCount,
	    SkeletonResource& skeleton,
	    std::vector<AnimationJointTransform>& bindLocal,
	    AnimationClipResource& clip)
	{
		skeleton.joints.resize(jointCount);
		bindLocal.resize(jointCount);
		clip.durationSeconds = 1.0f;
		clip.channels.reserve(jointCount);
		clip.keyframes.reserve(jointCount * 2u);
		for (std::uint32_t joint = 0; joint < jointCount; ++joint)
		{
			skeleton.joints[joint].parentJointIndex = joint == 0 ? (std::numeric_limits<std::uint32_t>::max)() : joint - 1;
			clip.channels.push_back(AnimationChannel{
			    .targetPath = Assets::CookedAnimationTargetPath::Translation,
			    .interpolation = Assets::CookedAnimationInterpolation::Linear,
			    .targetJointIndex = joint,
			    .firstKeyframe = joint * 2u,
			    .keyframeCount = 2});
			clip.keyframes.push_back(AnimationKeyframe{.timeSeconds = 0.0f, .value = {0.0f, 0.01f, 0.0f, 0.0f}});
			clip.keyframes.push_back(AnimationKeyframe{.timeSeconds = 1.0f, .value = {0.01f, 0.02f, 0.03f, 0.0f}});
		}
		clip.channelCount = static_cast<std::uint32_t>(clip.channels.size());
		clip.keyframeCount = static_cast<std::uint32_t>(clip.keyframes.size());
	}

	double MeasureAnimation(std::uint32_t workerCount, std::uint32_t instanceCount, std::uint32_t repetitions)
	{
		SkeletonResource skeleton;
		std::vector<AnimationJointTransform> bindLocal;
		AnimationClipResource clip;
		BuildAnimationFixture(64, skeleton, bindLocal, clip);
		const SkeletonEvaluationData skeletonData{&skeleton, bindLocal};
		std::vector<AnimationWorkSlot> slots(instanceCount);
		for (AnimationWorkSlot& slot : slots)
		{
			slot.Local.resize(64);
			slot.Model.resize(64);
			slot.Skinning.resize(64);
		}

		GameSystemDesc pose = MakeSystem("Validation.AnimationPose", GameSystemPhase::Animation);
		pose.Resources = {
		    {GameSystemResourceDomain::AnimationClips, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Write}};
		pose.Execution = Ranges(4, 8, 16);
		GameSystemDesc skinning = MakeSystem("Validation.Skinning", GameSystemPhase::Deformation);
		skinning.Resources = {
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkeletonResources, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Write}};
		skinning.Prerequisites = {pose.Id};
		skinning.Execution = Ranges(4, 8, 16);
		GameSystemGraph graph;
		graph.Add(pose);
		graph.Add(skinning);
		CompiledGameSystemGraph compiled = graph.Compile();
		Check(compiled.IsValid(), "animation benchmark graph compile");

		std::array<GameSystemExecutionBinding, 2> bindings{{
		    {pose.Id,
		     [instanceCount] { return instanceCount; },
		     [&](std::uint32_t begin, std::uint32_t end)
		     {
			     for (std::uint32_t index = begin; index < end; ++index)
			     {
				     if (!AnimationPoseEvaluator::Evaluate(
				             clip,
				             skeletonData,
				             static_cast<float>(index % 60u) / 60.0f,
				             slots[index].Local,
				             slots[index].Model))
					     return false;
			     }
			     return true;
		     }},
		    {skinning.Id,
		     [instanceCount] { return instanceCount; },
		     [&](std::uint32_t begin, std::uint32_t end)
		     {
			     for (std::uint32_t index = begin; index < end; ++index)
			     {
				     if (!SkinningMatrixEvaluator::Evaluate(skeletonData, slots[index].Model, slots[index].Skinning))
					     return false;
			     }
			     return true;
		     }}}};

		TaskExecutorConfig config{};
		if (workerCount != 0)
			config.FrameCriticalWorkerCount = workerCount;
		TaskExecutor executor(config);
		GameSystemGraphError error;
		compiled.Execute(executor, bindings, error);
		const auto start = Clock::now();
		for (std::uint32_t repetition = 0; repetition < repetitions; ++repetition)
			Check(compiled.Execute(executor, bindings, error), "animation benchmark execution");
		const auto elapsed = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
		return elapsed / repetitions;
	}

	void ValidateCrossoverAndCriticalPath()
	{
		const double tinySerial = MeasureAnimation(0, 1, 80);
		const double tinyParallel = MeasureAnimation(4, 1, 80);
		const double heavySerial = MeasureAnimation(0, 512, 8);
		const double heavyParallel = MeasureAnimation(4, 512, 8);
		Check(tinyParallel < 2.5, "tiny animation overhead budget");
		Check(heavyParallel < heavySerial, "animation-heavy parallel crossover");

		GameSystemDesc pose = MakeSystem("Validation.CriticalPose", GameSystemPhase::Animation);
		pose.Resources = {{GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Write}};
		pose.Execution = Ranges(1, 1, 1);
		GameSystemDesc morph = MakeSystem("Validation.CriticalMorph", GameSystemPhase::Animation);
		morph.Resources = {{GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Write}};
		morph.Execution = Ranges(1, 1, 1);
		GameSystemDesc commit = MakeSystem("Validation.CriticalCommit", GameSystemPhase::Deformation);
		commit.Resources = {
		    {GameSystemResourceDomain::PoseScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::MorphScratch, GameSystemAccessMode::Read},
		    {GameSystemResourceDomain::SkinningOutput, GameSystemAccessMode::Write}};
		commit.Prerequisites = {pose.Id, morph.Id};
		commit.Execution = Ranges(1, 1, 1);
		GameSystemGraph graph;
		graph.Add(pose);
		graph.Add(morph);
		graph.Add(commit);
		CompiledGameSystemGraph compiled = graph.Compile();
		std::array<GameSystemExecutionBinding, 3> bindings{{
		    {pose.Id,
		     [] { return 1u; },
		     [](std::uint32_t, std::uint32_t)
		     {
			     std::this_thread::sleep_for(std::chrono::milliseconds(20));
			     return true;
		     }},
		    {morph.Id,
		     [] { return 1u; },
		     [](std::uint32_t, std::uint32_t)
		     {
			     std::this_thread::sleep_for(std::chrono::milliseconds(20));
			     return true;
		     }},
		    {commit.Id, [] { return 1u; }, [](std::uint32_t, std::uint32_t) { return true; }}}};
		TaskExecutor executor(TaskExecutorConfig{.FrameCriticalWorkerCount = 2});
		GameSystemGraphError error;
		const auto start = Clock::now();
		Check(compiled.Execute(executor, bindings, error), "critical-path execution");
		const double criticalPath = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
		Check(criticalPath < 38.0, "independent animation branches overlap");

		std::cout << "tiny_serial_ms=" << tinySerial << " tiny_parallel_ms=" << tinyParallel
		          << " heavy_serial_ms=" << heavySerial << " heavy_parallel_ms=" << heavyParallel
		          << " critical_path_ms=" << criticalPath << '\n';
	}
}

int main()
{
	Filesystem::ConfigureProjectRoot(std::filesystem::current_path());
	ValidateCompilerRejections();
	ValidateFrozenStructureAndRetainedView();
	ValidateWorkerMatrix();
	ValidateMorphSampling();
	ValidateWorldParity();
	ValidateCrossoverAndCriticalPath();
	std::cout << (g_succeeded ? "Prompt10 validation PASSED\n" : "Prompt10 validation FAILED\n");
	return g_succeeded ? 0 : 1;
}
