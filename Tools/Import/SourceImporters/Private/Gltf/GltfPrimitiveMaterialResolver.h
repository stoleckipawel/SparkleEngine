#pragma once

#include "SourceImportOutput.h"

#include <string_view>

struct cgltf_data;
struct cgltf_primitive;

class GltfPrimitiveMaterialResolver final
{
  public:
	static ImportedMaterialIndex Resolve(
	    const cgltf_primitive& primitive,
	    const cgltf_data* data,
	    std::string_view primitiveLabel,
	    SourceImportOutput& output);
};
