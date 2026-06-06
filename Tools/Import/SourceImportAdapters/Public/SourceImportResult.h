#pragma once

#include "Types/ImportedScene.h"
#include "SourceImportDiagnostics.h"

#include <cstddef>
#include <string_view>

struct SourceImportResult
{
	ImportedScene scene;
	SourceImportDiagnostics diagnostics;
	bool succeeded = false;

	bool IsValid() const noexcept { return succeeded && !scene.meshPrimitives.empty() && !scene.meshInstances.empty(); }
	std::size_t GetMeshCount() const noexcept { return scene.GetMeshCount(); }
	std::size_t GetMeshPrimitiveCount() const noexcept { return scene.GetMeshPrimitiveCount(); }
	std::size_t GetMeshInstanceCount() const noexcept { return scene.GetMeshInstanceCount(); }
	std::size_t GetMeshInstanceGroupCount() const noexcept { return scene.GetMeshInstanceGroupCount(); }
	std::size_t GetCameraCount() const noexcept { return scene.GetCameraCount(); }
	std::size_t GetLightCount() const noexcept { return scene.GetLightCount(); }
	std::size_t GetMaterialCount() const noexcept { return scene.GetMaterialCount(); }
	std::string_view GetImporterName() const noexcept { return scene.importerName; }

	void ReserveMeshPrimitives(std::size_t primitiveCount)
	{
		scene.ReserveMeshPrimitives(primitiveCount);
	}

	void ReserveMeshInstances(std::size_t instanceCount)
	{
		scene.ReserveMeshInstances(instanceCount);
	}

	void ReserveMeshInstanceGroups(std::size_t instanceGroupCount)
	{
		scene.ReserveMeshInstanceGroups(instanceGroupCount);
	}
};
