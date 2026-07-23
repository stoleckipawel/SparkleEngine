#include "RenderWorldContract/RenderWorldContractFixture.h"

#include "Core/Public/Math/MathUtils.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"

class RenderWorldContractFixtureImplementation final
{
  public:
	class ValidationMesh final : public Mesh
	{
	  private:
		void GenerateGeometry(MeshData& data) const override
		{
			data.vertices = {VertexData({0.0f, 0.0f, 0.0f}), VertexData({1.0f, 0.0f, 0.0f}),
			                 VertexData({0.0f, 1.0f, 0.0f})};
			data.indices = {0, 1, 2};
		}
	};

	static RenderObjectStaticData MakeStatic(
	    const ImmutableRenderMeshHandle& mesh,
	    MaterialHandle material,
	    RenderSkeletonAssetHandle skeleton = {})
	{
		return RenderObjectStaticData{
		    .Mesh = mesh,
		    .Material = material,
		    .Skeleton = skeleton,
		    .MeshKind = skeleton.IsValid() ? SceneMeshKind::Skeletal : SceneMeshKind::Static};
	}

	static RenderInputFrame MakeResetFrame(
	    const ImmutableRenderMeshHandle& mesh,
	    RenderObjectId staticObject,
	    RenderObjectId skinnedObject)
	{
		const RenderSkeletonAssetHandle skeleton(200);
		const RenderAnimationAssetHandle animation(300);
		RenderInputFrame frame;
		frame.WorldDelta.SceneGeneration = 1;
		frame.WorldDelta.SequenceNumber = 1;
		frame.WorldDelta.ResetScene = true;
		frame.WorldDelta.Materials = RenderMaterialTable{.Values = {MaterialDesc{}, MaterialDesc{}}, .Generation = 1};
		frame.WorldDelta.Materials->Values[0].name = "mat-a";
		frame.WorldDelta.Materials->Values[1].name = "mat-b";
		frame.WorldDelta.Textures = RenderTextureTable{
		    .Assets = {{RenderTextureAssetHandle{0}, "textures/a.dds"}}, .Generation = 1};
		frame.WorldDelta.Sky.Published = true;
		frame.WorldDelta.Sky.Value = SceneSkyDesc{};
		frame.WorldDelta.InstanceGroups.Published = true;
		frame.WorldDelta.Creates.push_back({staticObject, MakeStatic(mesh, MaterialHandle(0, 1))});
		frame.WorldDelta.Creates.push_back({skinnedObject, MakeStatic(mesh, MaterialHandle(1, 1), skeleton)});
		frame.Dynamic.Metadata = MakeRenderWorldContractMetadata(1, 1);
		frame.Dynamic.Objects.push_back({.Object = staticObject});
		frame.Dynamic.Objects.push_back({.Object = skinnedObject});
		frame.Dynamic.Skinning.push_back(
		    {.Object = skinnedObject,
		     .Skeleton = skeleton,
		     .Animation = animation,
		     .Matrices = {MathUtils::IdentityFloat4x4()}});
		frame.Dynamic.MorphWeights.push_back(
		    {.Object = skinnedObject, .Animation = animation, .TargetNodeIndex = 4, .Weights = {0.25f, 0.75f}});
		SceneLightDesc light;
		light.payload = PointLightDesc{};
		frame.Dynamic.Lights.push_back({RenderObjectId::FromParts(100, 1), std::move(light)});
		return frame;
	}

	static std::vector<RenderInputFrame> BuildRecording(
	    const ImmutableRenderMeshHandle& mesh,
	    RenderObjectId staticObject,
	    RenderObjectId skinnedObject)
	{
		std::vector<RenderInputFrame> recording;
		recording.push_back(MakeResetFrame(mesh, staticObject, skinnedObject));

		RenderInputFrame update = recording.front();
		update.WorldDelta.SequenceNumber = 2;
		update.WorldDelta.ResetScene = false;
		update.WorldDelta.Creates.clear();
		update.WorldDelta.Materials.reset();
		update.WorldDelta.Textures.reset();
		update.WorldDelta.Sky = {};
		update.WorldDelta.InstanceGroups = {};
		update.WorldDelta.Updates.push_back({staticObject, MakeStatic(mesh, MaterialHandle(1, 1))});
		update.Dynamic.Metadata = MakeRenderWorldContractMetadata(2, 1);
		recording.push_back(std::move(update));

		RenderInputFrame destroy = recording.back();
		destroy.WorldDelta.SequenceNumber = 3;
		destroy.WorldDelta.Updates.clear();
		destroy.WorldDelta.Destroys.push_back(skinnedObject);
		destroy.WorldDelta.Sky.Published = true;
		destroy.WorldDelta.Sky.Value.reset();
		destroy.Dynamic.Metadata = MakeRenderWorldContractMetadata(3, 1);
		destroy.Dynamic.Objects.resize(1);
		destroy.Dynamic.Skinning.clear();
		destroy.Dynamic.MorphWeights.clear();
		recording.push_back(std::move(destroy));
		return recording;
	}
};

void RenderWorldContractFixture::ReleaseProducerOwnership() noexcept
{
	ProducerResource.reset();
	ProducerMesh = {};
}

RenderWorldContractFixture BuildRenderWorldContractFixture()
{
	RenderWorldContractFixture fixture;
	fixture.StaticObject = RenderObjectId::FromParts(7, 1);
	fixture.SkinnedObject = RenderObjectId::FromParts(8, 1);
	fixture.ProducerResource = std::make_shared<RenderWorldContractFixtureImplementation::ValidationMesh>();
	fixture.ProducerMesh = ImmutableRenderMeshHandle(42, fixture.ProducerResource);
	fixture.Recording = RenderWorldContractFixtureImplementation::BuildRecording(fixture.ProducerMesh, fixture.StaticObject, fixture.SkinnedObject);
	return fixture;
}

RenderFrameMetadata MakeRenderWorldContractMetadata(
    std::uint64_t frameId,
    std::uint64_t sceneGeneration,
    std::uint64_t providerGeneration)
{
	return RenderFrameMetadata{
	    .FrameId = frameId,
	    .FrameGeneration = sceneGeneration,
	    .ProviderGeneration = providerGeneration,
	    .RenderWidth = 960,
	    .RenderHeight = 540,
	    .OutputWidth = 1920,
	    .OutputHeight = 1080};
}
