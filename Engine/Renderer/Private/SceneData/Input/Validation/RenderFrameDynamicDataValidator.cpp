#include "PCH.h"
#include "SceneData/Input/Validation/RenderFrameDynamicDataValidator.h"

#include "SceneData/RenderWorld.h"

#include <map>
#include <set>
#include <tuple>

namespace
{
	using StaticDataByObject = std::map<RenderObjectId, const RenderObjectStaticData*>;

	StaticDataByObject BuildProjectedObjects(const RenderWorld& world, const RenderWorldDelta& delta)
	{
		StaticDataByObject objects;
		if (!delta.ResetScene)
			for (const auto& [object, proxy] : world.GetProxies()) objects.emplace(object, &proxy.Static);
		for (RenderObjectId object : delta.Destroys) objects.erase(object);
		for (const RenderObjectCreate& create : delta.Creates) objects[create.Object] = &create.Static;
		for (const RenderObjectUpdate& update : delta.Updates) objects[update.Object] = &update.Static;
		return objects;
	}
}

bool RenderFrameDynamicDataValidator::Validate(
    const RenderWorld& world,
    const RenderInputFrame& input,
    std::string& diagnostic)
{
	const StaticDataByObject projectedObjects = BuildProjectedObjects(world, input.WorldDelta);
	std::set<RenderObjectId> dynamicObjects;
	for (const RenderObjectDynamicData& object : input.Dynamic.Objects)
		if (!object.Object.IsValid() || !projectedObjects.contains(object.Object) ||
		    !dynamicObjects.insert(object.Object).second)
		{
			diagnostic = "Render input dynamic object rows contain an invalid, duplicate, or unavailable identity.";
			return false;
		}
	if (dynamicObjects.size() != projectedObjects.size())
	{
		diagnostic = "Render input dynamic object rows do not cover the projected render world exactly.";
		return false;
	}

	std::set<RenderObjectId> skinnedObjects;
	for (const RenderSkinningData& skinning : input.Dynamic.Skinning)
	{
		const auto object = projectedObjects.find(skinning.Object);
		if (object == projectedObjects.end() || !skinning.Skeleton.IsValid() || !skinning.Animation.IsValid() ||
		    skinning.Matrices.empty() || !skinnedObjects.insert(skinning.Object).second ||
		    object->second->Skeleton != skinning.Skeleton || object->second->MeshKind != SceneMeshKind::Skeletal)
		{
			diagnostic = "Render input skinning rows contain invalid identity, asset provenance, or matrix ranges.";
			return false;
		}
	}

	std::set<std::tuple<RenderObjectId, std::uint32_t>> morphTargets;
	for (const RenderMorphData& morph : input.Dynamic.MorphWeights)
		if (!projectedObjects.contains(morph.Object) || !morph.Animation.IsValid() || morph.Weights.empty() ||
		    morph.TargetNodeIndex == (std::numeric_limits<std::uint32_t>::max)() ||
		    !morphTargets.emplace(morph.Object, morph.TargetNodeIndex).second)
		{
			diagnostic = "Render input morph rows contain invalid identity, asset provenance, or weight ranges.";
			return false;
		}

	std::set<RenderObjectId> lights;
	for (const RenderLightData& light : input.Dynamic.Lights)
		if (!light.Object.IsValid() || !lights.insert(light.Object).second ||
		    light.Description.GetKind() == SceneLightKind::Unknown)
		{
			diagnostic = "Render input light rows contain an invalid or duplicate stable identity.";
			return false;
		}
	return true;
}
