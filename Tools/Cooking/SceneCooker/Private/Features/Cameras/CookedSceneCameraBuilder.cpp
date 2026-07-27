#include "PCH.h"

#include "CookedSceneCameraBuilder.h"

#include <algorithm>
#include <cstring>

class CookedCameraTranslation final
{
  public:
	static Assets::CookedSceneCameraProjectionKind ToCookedCameraProjectionKind(ImportedCameraProjectionKind projectionKind) noexcept
	{
		switch (projectionKind)
		{
			case ImportedCameraProjectionKind::Perspective:
				return Assets::CookedSceneCameraProjectionKind::Perspective;
			case ImportedCameraProjectionKind::Orthographic:
				return Assets::CookedSceneCameraProjectionKind::Orthographic;
			case ImportedCameraProjectionKind::Unknown:
			default:
				return Assets::CookedSceneCameraProjectionKind::Unknown;
		}
	}

	static Assets::CookedSceneCameraRecord BuildCameraRecord(const ImportedCamera& importedCamera)
	{
		Assets::CookedSceneCameraRecord cameraRecord;
		const std::size_t copyLength =
		    (std::min)(importedCamera.name.size(), static_cast<std::size_t>(Assets::kCookedSceneCameraNameCapacity - 1u));
		if (copyLength > 0)
		{
			std::memcpy(cameraRecord.name, importedCamera.name.data(), copyLength);
		}
		cameraRecord.worldTransform = importedCamera.worldTransform;
		cameraRecord.projectionKind = ToCookedCameraProjectionKind(importedCamera.projectionKind);
		cameraRecord.verticalFovRadians = importedCamera.verticalFovRadians;
		cameraRecord.nearPlane = importedCamera.nearPlane;
		cameraRecord.farPlane = importedCamera.farPlane;
		cameraRecord.sourceNodeIndex = importedCamera.sourceNodeIndex;
		return cameraRecord;
	}
};

void CookedSceneCameraBuilder::BuildCameras(const SourceImportResult& importResult, CookedSceneBuild& outBuild)
{
	outBuild.manifest.cameras.clear();
	outBuild.manifest.cameras.reserve(importResult.scene.cameras.size());

	for (const ImportedCamera& importedCamera : importResult.scene.cameras)
	{
		outBuild.manifest.cameras.push_back(CookedCameraTranslation::BuildCameraRecord(importedCamera));
	}
}
