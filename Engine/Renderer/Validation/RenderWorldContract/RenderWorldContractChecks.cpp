#include "RenderWorldContract/RenderWorldContractChecks.h"

#include "RenderWorldContract/RenderWorldContractFixture.h"
#include "GameFramework/Public/Scene/Meshes/Mesh.h"
#include "SceneData/Input/Validation/RenderInputFrameValidator.h"

#include <iostream>
#include <sstream>

namespace
{
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

	std::string Fingerprint(const RenderWorld& world)
	{
		std::ostringstream value;
		value << world.GetSceneGeneration() << ':' << world.GetSequenceNumber() << ':'
		      << world.GetMaterials().Generation << ':' << world.GetMaterials().Values.size() << ':'
		      << world.GetTextures().Generation << ':' << world.GetTextures().Assets.size() << ':'
		      << world.GetSky().has_value() << ':' << world.GetInstanceGroups().size();
		for (const auto& [object, proxy] : world.GetProxies())
			value << '|' << object.GetValue() << ':' << object.GetGeneration() << ':'
			      << proxy.Static.Mesh.GetAssetId() << ':'
			      << proxy.Static.Material.GetIndex() << ':' << proxy.Static.Material.GetGeneration() << ':'
			      << proxy.Static.Skeleton.GetAssetId();
		return value.str();
	}
}

bool ValidateDeterministicRenderWorldReplay(
    const std::vector<RenderInputFrame>& recording,
    RenderObjectId retainedObject,
    RenderWorld& retainedWorld,
    std::chrono::steady_clock::duration& replayElapsed)
{
	RenderWorld secondReplay;
	const auto replayStart = std::chrono::steady_clock::now();
	for (std::size_t index = 0; index < recording.size(); ++index)
	{
		if (!Expect(Apply(retainedWorld, recording[index].WorldDelta) == RenderWorldApplyStatus::Applied,
		            "First replay rejected a valid delta."))
			return false;
		if (!Expect(Apply(secondReplay, recording[index].WorldDelta) == RenderWorldApplyStatus::Applied,
		            "Second replay rejected a valid delta."))
			return false;
		if (index == 1 && !Expect(retainedWorld.GetSky().has_value(),
		                          "An omitted sky publication cleared retained sky state."))
			return false;
	}
	replayElapsed = std::chrono::steady_clock::now() - replayStart;
	if (!Expect(Fingerprint(retainedWorld) == Fingerprint(secondReplay),
	            "Recorded replay produced different render-world state."))
		return false;
	return Expect(retainedWorld.GetProxies().size() == 1 && retainedWorld.Find(retainedObject) != nullptr &&
	                  !retainedWorld.GetSky(),
	              "Create/update/destroy replay did not reach the expected final state.");
}

bool ValidateRenderWorldOrdering(const std::vector<RenderInputFrame>& recording)
{
	RenderWorld world;
	if (!Expect(Apply(world, recording[0].WorldDelta) == RenderWorldApplyStatus::Applied, "Initial reset was rejected."))
		return false;
	if (!Expect(Apply(world, recording[0].WorldDelta) == RenderWorldApplyStatus::Duplicate,
	            "Duplicate delta was not rejected."))
		return false;
	RenderWorldDelta gap = recording[2].WorldDelta;
	if (!Expect(Apply(world, gap) == RenderWorldApplyStatus::OutOfOrder, "Sequence gap was not rejected.")) return false;

	RenderWorldDelta newerWithoutReset;
	newerWithoutReset.SceneGeneration = 2;
	newerWithoutReset.SequenceNumber = 4;
	if (!Expect(Apply(world, newerWithoutReset) == RenderWorldApplyStatus::Rejected,
	            "New generation without reset was accepted."))
		return false;

	RenderWorldDelta incompleteReset;
	incompleteReset.SceneGeneration = 2;
	incompleteReset.SequenceNumber = 4;
	incompleteReset.ResetScene = true;
	incompleteReset.Materials = RenderMaterialTable{.Generation = 2};
	incompleteReset.Textures = RenderTextureTable{.Generation = 2};
	if (!Expect(Apply(world, incompleteReset) == RenderWorldApplyStatus::Rejected,
	            "Incomplete scene publication was accepted."))
		return false;

	RenderWorldDelta nextGeneration = incompleteReset;
	nextGeneration.Sky.Published = true;
	nextGeneration.InstanceGroups.Published = true;
	if (!Expect(Apply(world, nextGeneration) == RenderWorldApplyStatus::Applied,
	            "Valid next-generation reset was rejected."))
		return false;

	RenderWorldDelta stale;
	stale.SceneGeneration = 1;
	stale.SequenceNumber = 2;
	return Expect(Apply(world, stale) == RenderWorldApplyStatus::Stale, "Stale generation was not rejected.");
}

bool ValidateRejectedRenderWorldDeltas(const RenderInputFrame& resetFrame, RenderObjectId object)
{
	RenderWorld duplicateCreateWorld;
	RenderWorldDelta duplicateCreate = resetFrame.WorldDelta;
	duplicateCreate.Creates.push_back(duplicateCreate.Creates.front());
	if (!Expect(Apply(duplicateCreateWorld, duplicateCreate) == RenderWorldApplyStatus::Rejected &&
	                duplicateCreateWorld.GetProxies().empty(),
	            "Duplicate creates were not rejected before render-world mutation."))
		return false;

	RenderWorld invalidTextureWorld;
	RenderWorldDelta invalidTexture = resetFrame.WorldDelta;
	invalidTexture.Textures->Assets.front().Handle.Index = 99;
	if (!Expect(Apply(invalidTextureWorld, invalidTexture) == RenderWorldApplyStatus::Rejected,
	            "Invalid texture generation was accepted."))
		return false;

	const auto rejects = [&resetFrame](RenderWorldDelta invalid)
	{
		RenderWorld world;
		if (Apply(world, resetFrame.WorldDelta) != RenderWorldApplyStatus::Applied) return false;
		return Apply(world, invalid) == RenderWorldApplyStatus::Rejected && world.GetSequenceNumber() == 1;
	};

	RenderWorldDelta duplicateUpdate;
	duplicateUpdate.SceneGeneration = 1;
	duplicateUpdate.SequenceNumber = 2;
	duplicateUpdate.Updates = {{object, resetFrame.WorldDelta.Creates[0].Static},
	                           {object, resetFrame.WorldDelta.Creates[0].Static}};
	if (!Expect(rejects(duplicateUpdate), "Duplicate updates were not rejected atomically.")) return false;

	RenderWorldDelta updateAndDestroy = duplicateUpdate;
	updateAndDestroy.Updates.resize(1);
	updateAndDestroy.Destroys.push_back(object);
	if (!Expect(rejects(updateAndDestroy), "Conflicting update/destroy operations were not rejected atomically."))
		return false;

	RenderWorldDelta duplicateDestroy;
	duplicateDestroy.SceneGeneration = 1;
	duplicateDestroy.SequenceNumber = 2;
	duplicateDestroy.Destroys = {object, object};
	if (!Expect(rejects(duplicateDestroy), "Duplicate destroys were not rejected atomically.")) return false;

	RenderWorldDelta wrongMaterial = duplicateUpdate;
	wrongMaterial.Updates.resize(1);
	wrongMaterial.Updates[0].Static.Material = MaterialHandle(0, 99);
	return Expect(rejects(wrongMaterial), "Mismatched material generation was not rejected.");
}

bool ValidateRenderInputFrameAdmission(const std::vector<RenderInputFrame>& recording)
{
	RenderWorld world;
	RenderInputFrameValidator validator;
	for (const RenderInputFrame& frame : recording)
	{
		std::string diagnostic;
		bool reset = false;
		if (!Expect(world.Validate(frame.WorldDelta, diagnostic) == RenderWorldApplyStatus::Applied,
		            "A valid frame delta failed preflight validation.") ||
		    !Expect(validator.Validate(world, frame, reset, diagnostic),
		            "A valid dynamic frame failed admission validation."))
			return false;
		if (!Expect(Apply(world, frame.WorldDelta) == RenderWorldApplyStatus::Applied,
		            "A validated frame failed application."))
			return false;
		validator.Commit(frame.Dynamic.Metadata);
	}

	RenderInputFrame duplicateFrame = recording.back();
	duplicateFrame.WorldDelta.SequenceNumber = 4;
	std::string diagnostic;
	bool reset = false;
	if (!Expect(!validator.Validate(world, duplicateFrame, reset, diagnostic), "Duplicate frame identity was accepted."))
		return false;

	RenderInputFrame missingDynamic = recording.back();
	missingDynamic.WorldDelta.SequenceNumber = 4;
	missingDynamic.Dynamic.Metadata = MakeRenderWorldContractMetadata(4, 1);
	missingDynamic.Dynamic.Objects.clear();
	if (!Expect(!validator.Validate(world, missingDynamic, reset, diagnostic),
	            "Incomplete dynamic object coverage was accepted."))
		return false;

	RenderInputFrame providerChange = recording.back();
	providerChange.WorldDelta.SequenceNumber = 4;
	providerChange.Dynamic.Metadata = MakeRenderWorldContractMetadata(4, 1, 8);
	if (!Expect(validator.Validate(world, providerChange, reset, diagnostic) && reset,
	            "Provider generation change did not request history reset."))
		return false;

	RenderInputFrame cutWithoutReset = providerChange;
	cutWithoutReset.Dynamic.Metadata.CameraCut = true;
	cutWithoutReset.Dynamic.Metadata.ResetHistory = false;
	if (!Expect(!validator.Validate(world, cutWithoutReset, reset, diagnostic),
	            "Camera cut without history reset was accepted."))
		return false;

	return true;
}

bool ValidateRetainedRenderWorldOwnership(const RenderWorld& world, RenderObjectId retainedObject)
{
	const RenderProxy* retained = world.Find(retainedObject);
	return Expect(retained != nullptr && retained->Static.Mesh.IsValid() &&
	                  retained->Static.Mesh.GetResource()->GetMeshData().IsValid() &&
	                  world.GetMaterials().Values.size() == 2 && world.GetTextures().Assets.size() == 1,
	              "Render world lost immutable values/handles after producer and acknowledged packet storage were poisoned.");
}
