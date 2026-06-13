#pragma once

#include "SourceImportResult.h"

struct cgltf_data;

class GltfCameraImporter final
{
  public:
	static void ImportCameras(const cgltf_data* data, SourceImportResult& result);
};
