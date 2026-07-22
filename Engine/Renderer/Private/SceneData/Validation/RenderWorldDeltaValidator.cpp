#include "PCH.h"
#include "SceneData/Validation/RenderWorldDeltaValidator.h"

#include <set>

RenderWorldApplyStatus RenderWorldDeltaValidator::Validate(
    const RenderWorld& world,
    const RenderWorldDelta& delta,
    std::string& diagnostic)
{
	const RenderWorldApplyStatus sequenceStatus = ValidateSequence(world, delta, diagnostic);
	if (sequenceStatus != RenderWorldApplyStatus::Applied) return sequenceStatus;
	if (!ValidateResources(world, delta, diagnostic) || !ValidateOperations(world, delta, diagnostic))
		return RenderWorldApplyStatus::Rejected;
	diagnostic.clear();
	return RenderWorldApplyStatus::Applied;
}

RenderWorldApplyStatus RenderWorldDeltaValidator::ValidateSequence(
    const RenderWorld& world,
    const RenderWorldDelta& delta,
    std::string& diagnostic)
{
	if (delta.SceneGeneration == 0 || delta.SequenceNumber == 0)
	{
		diagnostic = "Render-world scene generation and sequence must be non-zero.";
		return RenderWorldApplyStatus::Rejected;
	}
	if (delta.SceneGeneration < world.GetSceneGeneration())
	{
		diagnostic = "Render-world delta belongs to a stale scene generation.";
		return RenderWorldApplyStatus::Stale;
	}
	if (delta.SceneGeneration == world.GetSceneGeneration() && delta.SequenceNumber == world.GetSequenceNumber())
	{
		diagnostic = "Render-world delta is a duplicate.";
		return RenderWorldApplyStatus::Duplicate;
	}
	if (delta.SceneGeneration == world.GetSceneGeneration() && world.GetSequenceNumber() != 0 &&
	    delta.SequenceNumber != world.GetSequenceNumber() + 1)
	{
		diagnostic = "Render-world delta is out of order.";
		return RenderWorldApplyStatus::OutOfOrder;
	}
	if (delta.SceneGeneration > world.GetSceneGeneration() && !delta.ResetScene)
	{
		diagnostic = "A new render scene generation must begin with ResetScene.";
		return RenderWorldApplyStatus::Rejected;
	}
	return RenderWorldApplyStatus::Applied;
}

bool RenderWorldDeltaValidator::ValidateOperations(
    const RenderWorld& world,
    const RenderWorldDelta& delta,
    std::string& diagnostic)
{
	const bool reset = delta.ResetScene;
	std::set<RenderObjectId> destroys;
	for (RenderObjectId object : delta.Destroys)
	{
		if (!object.IsValid() || reset || world.Find(object) == nullptr || !destroys.insert(object).second)
		{
			diagnostic = "Render-world destroy contained an invalid, duplicate, or unavailable object.";
			return false;
		}
	}

	std::set<RenderObjectId> creates;
	for (const RenderObjectCreate& create : delta.Creates)
	{
		if (!create.Object.IsValid() || !create.Static.Mesh.IsValid() || (!reset && world.Find(create.Object) != nullptr) ||
		    !creates.insert(create.Object).second)
		{
			diagnostic = "Render-world create contained an invalid or duplicate identity/asset handle.";
			return false;
		}
	}

	std::set<RenderObjectId> updates;
	for (const RenderObjectUpdate& update : delta.Updates)
	{
		if (!update.Object.IsValid() || !update.Static.Mesh.IsValid() || reset || world.Find(update.Object) == nullptr ||
		    destroys.contains(update.Object) || creates.contains(update.Object) || !updates.insert(update.Object).second)
		{
			diagnostic = "Render-world update contained an invalid, duplicate, conflicting, or unavailable object.";
			return false;
		}
	}
	return true;
}

bool RenderWorldDeltaValidator::ValidateResources(
    const RenderWorld& world,
    const RenderWorldDelta& delta,
    std::string& diagnostic)
{
	if (delta.ResetScene && (!delta.Materials || !delta.Textures || !delta.Sky.Published || !delta.InstanceGroups.Published))
	{
		diagnostic = "Render-world reset must publish complete material, texture, sky, and instance-group state.";
		return false;
	}
	if ((delta.Materials && delta.Materials->Generation == 0) || (delta.Textures && delta.Textures->Generation == 0))
	{
		diagnostic = "Render-world resource-table generation must be non-zero.";
		return false;
	}

	if (delta.Textures)
		for (std::uint32_t index = 0; index < delta.Textures->Assets.size(); ++index)
		{
			const RenderTextureAsset& texture = delta.Textures->Assets[index];
			if (!texture.Handle.IsValid() || texture.Handle.Index != index || texture.Path.empty())
			{
				diagnostic = "Render-world texture table contains an invalid handle or path.";
				return false;
			}
		}

	const std::uint32_t materialGeneration =
	    delta.Materials ? delta.Materials->Generation : world.GetMaterials().Generation;
	const std::size_t materialCount =
	    delta.Materials ? delta.Materials->Values.size() : world.GetMaterials().Values.size();
	const auto staticDataIsValid = [materialGeneration, materialCount](const RenderObjectStaticData& data)
	{
		if (!data.Mesh.IsValid()) return false;
		if (!data.Material.IsValid() || data.Material.GetGeneration() != materialGeneration ||
		    data.Material.GetIndex() >= materialCount)
			return false;
		if (data.MeshKind == SceneMeshKind::Skeletal) return data.Skeleton.IsValid();
		return !data.Skeleton.IsValid();
	};

	for (const RenderObjectCreate& create : delta.Creates)
		if (!staticDataIsValid(create.Static))
		{
			diagnostic = "Render-world create referenced an incompatible resource generation or mesh classification.";
			return false;
		}
	for (const RenderObjectUpdate& update : delta.Updates)
		if (!staticDataIsValid(update.Static))
		{
			diagnostic = "Render-world update referenced an incompatible resource generation or mesh classification.";
			return false;
		}

	if (delta.InstanceGroups.Published)
		for (const RenderMeshInstanceGroupData& group : delta.InstanceGroups.Values)
			if (!group.Material.IsValid() || group.Material.GetGeneration() != materialGeneration ||
			    group.Material.GetIndex() >= materialCount)
			{
				diagnostic = "Render-world instance group referenced an incompatible material generation.";
				return false;
			}
	return true;
}
