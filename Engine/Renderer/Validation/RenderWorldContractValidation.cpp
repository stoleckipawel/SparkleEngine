#include "SceneData/RenderWorld.h"

#include "GameFramework/Public/Rendering/RenderInputFrame.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace
{
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

	bool Expect(bool condition, const char* message)
	{
		if (!condition) std::cerr << message << '\n';
		return condition;
	}

	RenderWorldApplyStatus Apply(RenderWorld& world, const RenderWorldDelta& delta)
	{
		std::string diagnostic;
		return world.Apply(delta, diagnostic);
	}
}

int main()
{
	const RenderObjectId object = RenderObjectId::FromParts(7, 1);
	auto sourceMesh = std::make_shared<ValidationMesh>();
	ImmutableRenderMeshHandle mesh(42, 1, sourceMesh);

	std::vector<RenderInputFrame> recording(3);
	recording[0].WorldDelta.SceneGeneration = 1;
	recording[0].WorldDelta.SequenceNumber = 1;
	recording[0].WorldDelta.ResetScene = true;
	recording[0].WorldDelta.Materials = RenderMaterialTable{.Generation = 1};
	recording[0].WorldDelta.Textures = RenderTextureTable{.Generation = 1};
	recording[0].WorldDelta.Creates.push_back({.Object = object, .Mesh = mesh, .Material = MaterialHandle(0, 1)});
	recording[1].WorldDelta.SceneGeneration = 1;
	recording[1].WorldDelta.SequenceNumber = 2;
	recording[1].WorldDelta.Updates.push_back({.Object = object, .Material = MaterialHandle(1, 1)});
	recording[2].WorldDelta.SceneGeneration = 1;
	recording[2].WorldDelta.SequenceNumber = 3;
	recording[2].WorldDelta.Destroys.push_back(object);

	// The packet owns the immutable resource lifetime after the producer releases its copy.
	sourceMesh.reset();
	mesh = {};
	RenderWorld firstReplay;
	RenderWorld secondReplay;
	for (const RenderInputFrame& frame : recording)
	{
		if (!Expect(Apply(firstReplay, frame.WorldDelta) == RenderWorldApplyStatus::Applied, "First replay rejected a valid delta.")) return 1;
		if (!Expect(Apply(secondReplay, frame.WorldDelta) == RenderWorldApplyStatus::Applied, "Second replay rejected a valid delta.")) return 2;
	}
	if (!Expect(firstReplay.GetSceneGeneration() == secondReplay.GetSceneGeneration() &&
	            firstReplay.GetSequenceNumber() == secondReplay.GetSequenceNumber() &&
	            firstReplay.GetProxies().size() == secondReplay.GetProxies().size(),
	            "Recorded replay produced different final state.")) return 3;
	if (!Expect(firstReplay.GetProxies().empty() && firstReplay.GetSequenceNumber() == 3, "Destroy replay did not reach the expected final state.")) return 4;

	RenderWorld ordering;
	if (!Expect(Apply(ordering, recording[0].WorldDelta) == RenderWorldApplyStatus::Applied, "Initial reset was rejected.")) return 5;
	if (!Expect(Apply(ordering, recording[0].WorldDelta) == RenderWorldApplyStatus::Duplicate, "Duplicate delta was not rejected.")) return 6;
	RenderWorldDelta gap = recording[2].WorldDelta;
	if (!Expect(Apply(ordering, gap) == RenderWorldApplyStatus::OutOfOrder, "Sequence gap was not rejected.")) return 7;
	RenderWorldDelta newerWithoutReset;
	newerWithoutReset.SceneGeneration = 2;
	newerWithoutReset.SequenceNumber = 4;
	if (!Expect(Apply(ordering, newerWithoutReset) == RenderWorldApplyStatus::Rejected, "New generation without reset was accepted.")) return 8;
	RenderWorldDelta nextGeneration;
	nextGeneration.SceneGeneration = 2;
	nextGeneration.SequenceNumber = 4;
	nextGeneration.ResetScene = true;
	nextGeneration.Materials = RenderMaterialTable{.Generation = 2};
	nextGeneration.Textures = RenderTextureTable{.Generation = 2};
	if (!Expect(Apply(ordering, nextGeneration) == RenderWorldApplyStatus::Applied, "Valid next-generation reset was rejected.")) return 9;
	RenderWorldDelta stale;
	stale.SceneGeneration = 1;
	stale.SequenceNumber = 2;
	if (!Expect(Apply(ordering, stale) == RenderWorldApplyStatus::Stale, "Stale generation was not rejected.")) return 10;

	RenderWorld delayedConsumer;
	if (!Expect(Apply(delayedConsumer, recording[0].WorldDelta) == RenderWorldApplyStatus::Applied, "Delayed consumer rejected the recorded reset.")) return 11;
	recording.clear();
	const RenderProxy* retained = delayedConsumer.Find(object);
	if (!Expect(retained != nullptr && retained->Mesh.IsValid() && retained->Mesh.GetResource()->GetMeshData().IsValid(),
	            "Render proxy did not retain its immutable mesh after packet acknowledgement.")) return 12;

	std::cout << "RenderWorld contract validation passed\n";
	return 0;
}
