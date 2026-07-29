#pragma once

#include "SourceImportOutput.h"

struct cgltf_data;

class GltfCameraImporter final
{
  public:
	static void ImportCameras(const cgltf_data* data, SourceImportOutput& output);
};
