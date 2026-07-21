#include "PCH.h"
#include "World/Extraction/RenderInputExtractor.h"

#include "World/GameWorldState.h"
#include "World/ECS/Components/AnimationComponents.h"
#include "World/Resources/GameWorldResourceStores.h"
#include "World/WorldReadView.h"

namespace ECS
{
	RenderInputFrame RenderInputExtractor::Extract(
	    GameWorldState& state,
	    GameWorldResourceStores& resources,
	    const WorldReadView& readView,
	    std::uint64_t sceneGeneration,
	    RenderFrameMetadata metadata)
	{
		RenderInputFrame frame;
		BeginSceneFrame(frame, sceneGeneration, metadata);
		ExtractObjects(state, frame);
		ExtractResources(state, resources, frame.WorldDelta);
		ExtractDynamicState(state, readView, frame.Dynamic);
		return frame;
	}

	void RenderInputExtractor::BeginSceneFrame(
	    RenderInputFrame& frame, std::uint64_t sceneGeneration, RenderFrameMetadata metadata)
	{
		frame.Dynamic.Metadata = metadata;
		frame.Dynamic.Metadata.FrameGeneration = sceneGeneration;
		frame.WorldDelta.SceneGeneration = sceneGeneration;
		frame.WorldDelta.SequenceNumber = ++m_sequence;
		frame.WorldDelta.ResetScene = m_sceneGeneration != sceneGeneration;
		frame.Dynamic.Metadata.CameraCut |= frame.WorldDelta.ResetScene;
		frame.Dynamic.Metadata.ResetHistory |= frame.WorldDelta.ResetScene;
		if (!frame.WorldDelta.ResetScene) return;
		m_objects.clear();
		m_nextObjectValue = 0;
		m_sceneGeneration = sceneGeneration;
		m_publishedMaterialRevision = 0;
		m_publishedTextureRevision = 0;
		m_publishedSkyTexturePath.reset();
	}

	void RenderInputExtractor::ExtractObjects(GameWorldState& state, RenderInputFrame& frame)
	{
		std::map<EntityId, PublishedObject> nextObjects;
		const auto extracted = state.GetExtractedMeshes();
		frame.Dynamic.Objects.reserve(extracted.size());
		for (const WorldExtractionStorage::MeshSlot& mesh : extracted)
		{
			auto previous = m_objects.find(mesh.Entity);
			PublishedObject published;
			if (previous == m_objects.end())
			{
				published.Object = RenderObjectId::FromParts(m_nextObjectValue++, static_cast<std::uint32_t>(m_sceneGeneration));
				published.Material = mesh.Material;
				published.InstanceGroupIndex = mesh.InstanceGroupIndex;
				frame.WorldDelta.Creates.push_back(
				    {.Object = published.Object, .Mesh = mesh.Mesh, .Material = mesh.Material,
				     .SkeletonAssetId = mesh.SkeletonAssetId, .MeshKind = mesh.Kind,
				     .MeshAssetIndex = mesh.MeshAssetIndex, .InstanceGroupIndex = mesh.InstanceGroupIndex});
			}
			else
			{
				published = previous->second;
				if (published.Material != mesh.Material || published.InstanceGroupIndex != mesh.InstanceGroupIndex)
				{
					published.Material = mesh.Material;
					published.InstanceGroupIndex = mesh.InstanceGroupIndex;
					frame.WorldDelta.Updates.push_back({published.Object, mesh.Material, mesh.InstanceGroupIndex});
				}
			}
			nextObjects.emplace(mesh.Entity, published);
			frame.Dynamic.Objects.push_back({published.Object, mesh.WorldMatrix, mesh.WorldInverseTranspose, mesh.Visible});
		}
		for (const auto& [entity, previous] : m_objects)
			if (!nextObjects.contains(entity)) frame.WorldDelta.Destroys.push_back(previous.Object);
		m_objects = std::move(nextObjects);
	}

	void RenderInputExtractor::ExtractResources(
	    GameWorldState& state, GameWorldResourceStores& resources, RenderWorldDelta& delta)
	{
		ExtractInstanceGroups(state, delta);
		const std::optional<SkyEnvironment> sky = state.ReadSkyEnvironment();
		delta.Sky = sky ? std::optional<SceneSkyDesc>(sky->Description) : std::nullopt;

		std::optional<std::filesystem::path> skyTexturePath;
		if (sky && sky->Description.skyTexture.IsValid())
			skyTexturePath.emplace(sky->Description.skyTexture.texturePath);
		PublishChangedResourceTables(resources, skyTexturePath, delta);
	}

	void RenderInputExtractor::ExtractInstanceGroups(GameWorldState& state, RenderWorldDelta& delta)
	{
		for (const SceneMeshInstanceGroupData& group : state.GetExtractedMeshGroups())
			delta.InstanceGroups.push_back({group.meshAssetId, group.meshAssetIndex, group.materialHandle,
			                                group.firstInstance, group.instanceCount, group.groupKind, group.flags});
	}

	void RenderInputExtractor::PublishChangedResourceTables(
	    GameWorldResourceStores& resources,
	    const std::optional<std::filesystem::path>& skyTexturePath,
	    RenderWorldDelta& delta)
	{
		const std::uint64_t materialRevision = resources.Materials.GetContentRevision();
		if (delta.ResetScene || materialRevision != m_publishedMaterialRevision)
		{
			delta.Materials = resources.Materials.CaptureRenderTable();
			m_publishedMaterialRevision = materialRevision;
		}

		const std::uint64_t textureRevision = resources.Textures.GetContentRevision();
		if (delta.ResetScene || textureRevision != m_publishedTextureRevision ||
		    skyTexturePath != m_publishedSkyTexturePath)
		{
			if (skyTexturePath)
				delta.Textures = resources.Textures.CaptureRenderTable(
				    std::span<const std::filesystem::path>(&*skyTexturePath, 1));
			else
				delta.Textures = resources.Textures.CaptureRenderTable();
			m_publishedTextureRevision = textureRevision;
			m_publishedSkyTexturePath = skyTexturePath;
		}
	}

	void RenderInputExtractor::ExtractDynamicState(
	    GameWorldState& state, const WorldReadView& readView, RenderFrameDynamicData& dynamic) const
	{
		dynamic.Camera = BuildCamera(readView);
		dynamic.Lights = state.CaptureLightsToDesc();
		const AnimationOutput& output = state.m_animationOutput.GetOutput();
		for (const auto& [entity, published] : m_objects)
		{
			const SkinningState* skinning = state.m_registry.Get<SkinningState>(entity);
			if (skinning == nullptr || !skinning->Pose.IsValid() ||
			    skinning->Pose.Generation != state.m_animationOutput.GetTargetGeneration() ||
			    skinning->Pose.Slot >= output.poses.size())
				continue;
			const AnimationPoseOutput& pose = output.poses[skinning->Pose.Slot];
			dynamic.Skinning.push_back({published.Object, pose.skeletonAssetId, pose.skinningMatrices});
		}

		const auto samples = state.m_animationOutput.GetMorphSamples();
		for (const AnimationOutputStorage::MorphTargetBinding& binding : state.m_animationOutput.GetMorphBindings())
		{
			if (binding.SampleIndex >= samples.size()) continue;
			const std::uint32_t outputIndex = samples[binding.SampleIndex].OutputIndex;
			if (outputIndex >= output.morphWeights.size()) continue;
			const auto object = m_objects.find(binding.TargetEntity);
			if (object == m_objects.end()) continue;
			const MorphWeightOutput& morph = output.morphWeights[outputIndex];
			dynamic.MorphWeights.push_back({object->second.Object, morph.targetNodeIndex, morph.weights});
		}
	}

	RenderCameraData RenderInputExtractor::BuildCamera(const WorldReadView& readView) noexcept
	{
		for (const WorldCameraReadData& camera : readView.GetCameras())
		{
			if (!camera.Active) continue;
			return {.Position = camera.LocalTransform.GetTranslation(), .Direction = camera.Direction,
			        .FovYDegrees = camera.Description.fovYDegrees, .AspectRatio = camera.AspectRatio,
			        .NearZ = camera.Description.nearZ, .FarZ = camera.Description.farZ};
		}
		return {};
	}
}
