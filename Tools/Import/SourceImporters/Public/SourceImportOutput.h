#pragma once

#include "Types/ImportedScene.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>

struct SourceImportProvenance
{
	std::filesystem::path sourcePath;
	std::string importerId;
	float sourceMetersPerUnit = 0.0f;
};

struct SourceImportOutput
{
	ImportedScene scene;
	SourceImportProvenance provenance;

	std::size_t GetMeshPrimitiveCount() const noexcept { return scene.GetMeshPrimitiveCount(); }
	std::size_t GetMeshInstanceCount() const noexcept { return scene.GetMeshInstanceCount(); }
	std::size_t GetMeshInstanceGroupCount() const noexcept { return scene.GetMeshInstanceGroupCount(); }
	std::size_t GetCameraCount() const noexcept { return scene.GetCameraCount(); }
	std::size_t GetLightCount() const noexcept { return scene.GetLightCount(); }
	std::size_t GetAnimationCount() const noexcept { return scene.GetAnimationCount(); }
	std::size_t GetMaterialCount() const noexcept { return scene.GetMaterialCount(); }
	std::size_t GetMaterialVariantCount() const noexcept { return scene.GetMaterialVariantCount(); }
	std::size_t GetMaterialVariantMappingCount() const noexcept { return scene.GetMaterialVariantMappingCount(); }
	std::string_view GetImporterId() const noexcept { return provenance.importerId; }
	const std::filesystem::path& GetSourcePath() const noexcept { return provenance.sourcePath; }
	bool HasCanonicalCoordinates() const noexcept { return scene.HasCanonicalCoordinates(); }

	void ReserveMeshPrimitives(std::size_t primitiveCount) { scene.ReserveMeshPrimitives(primitiveCount); }

	void ReserveMeshInstances(std::size_t instanceCount) { scene.ReserveMeshInstances(instanceCount); }

	void ReserveMeshInstanceGroups(std::size_t instanceGroupCount) { scene.ReserveMeshInstanceGroups(instanceGroupCount); }
};
