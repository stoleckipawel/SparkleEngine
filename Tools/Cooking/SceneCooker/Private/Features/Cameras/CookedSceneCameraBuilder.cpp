#include "PCH.h"

#include "CookedSceneCameraBuilder.h"

#include "Core/Public/Diagnostics/Error.h"

#include <cstring>
#include <format>

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
		if (!importedCamera.name.empty())
		{
			std::memcpy(cameraRecord.name, importedCamera.name.data(), importedCamera.name.size());
		}
		cameraRecord.worldTransform = importedCamera.worldTransform;
		cameraRecord.projectionKind = ToCookedCameraProjectionKind(importedCamera.projectionKind);
		cameraRecord.fovYRadians = importedCamera.fovYRadians;
		cameraRecord.nearZ = importedCamera.nearZ;
		cameraRecord.farZ = importedCamera.farZ;
		cameraRecord.sourceNodeIndex = importedCamera.sourceNodeIndex;
		return cameraRecord;
	}
};

void CookedSceneCameraBuilder::BuildCameras(const SourceImportOutput& importOutput, CookedSceneBuild& outBuild)
{
	outBuild.manifest.cameras.clear();
	outBuild.manifest.cameras.reserve(importOutput.scene.cameras.size());

	for (std::size_t cameraIndex = 0; cameraIndex < importOutput.scene.cameras.size(); ++cameraIndex)
	{
		const ImportedCamera& importedCamera = importOutput.scene.cameras[cameraIndex];
		if (importedCamera.name.size() >= Assets::kCookedSceneCameraNameCapacity
		    || importedCamera.projectionKind != ImportedCameraProjectionKind::Perspective)
		{
			throw Diagnostics::Error(std::format("Imported camera {} exceeds the cooked camera contract.", cameraIndex));
		}
		outBuild.manifest.cameras.push_back(CookedCameraTranslation::BuildCameraRecord(importedCamera));
	}
}
