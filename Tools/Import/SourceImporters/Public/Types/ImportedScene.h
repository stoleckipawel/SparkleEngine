#pragma once

#include "ImportedCamera.h"
#include "ImportedAnimation.h"
#include "ImportedGeometry.h"
#include "ImportedLight.h"
#include "ImportedMaterial.h"
#include "ImportedMaterialVariant.h"
#include "ImportedSkin.h"

#include <cstddef>
#include <vector>

struct ImportedScene
{
	std::vector<ImportedMeshPrimitive> meshPrimitives;
	std::vector<ImportedMeshInstance> meshInstances;
	std::vector<ImportedMeshInstanceGroup> meshInstanceGroups;
	std::vector<ImportedCamera> cameras;
	std::vector<ImportedLight> lights;
	std::vector<ImportedAnimationClip> animations;
	std::vector<ImportedMaterial> materials;
	std::vector<ImportedMaterialVariant> materialVariants;
	std::vector<ImportedMaterialVariantMapping> materialVariantMappings;
	std::vector<ImportedSkeleton> skeletons;

	std::size_t GetMeshPrimitiveCount() const noexcept { return meshPrimitives.size(); }
	std::size_t GetMeshInstanceCount() const noexcept { return meshInstances.size(); }
	std::size_t GetMeshInstanceGroupCount() const noexcept { return meshInstanceGroups.size(); }
	std::size_t GetCameraCount() const noexcept { return cameras.size(); }
	std::size_t GetLightCount() const noexcept { return lights.size(); }
	std::size_t GetAnimationCount() const noexcept { return animations.size(); }
	std::size_t GetMaterialCount() const noexcept { return materials.size(); }
	std::size_t GetMaterialVariantCount() const noexcept { return materialVariants.size(); }
	std::size_t GetMaterialVariantMappingCount() const noexcept { return materialVariantMappings.size(); }
	std::size_t GetSkeletonCount() const noexcept { return skeletons.size(); }

	void ReserveMeshPrimitives(std::size_t primitiveCount) { meshPrimitives.reserve(primitiveCount); }

	void ReserveMeshInstances(std::size_t instanceCount) { meshInstances.reserve(instanceCount); }

	void ReserveMeshInstanceGroups(std::size_t instanceGroupCount) { meshInstanceGroups.reserve(instanceGroupCount); }
};
