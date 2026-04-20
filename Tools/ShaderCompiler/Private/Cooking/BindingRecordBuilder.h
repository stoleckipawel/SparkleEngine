#pragma once

#include "Core/Public/Strings/StringTableBuilder.h"
#include "RHI/Public/Shaders/CookedShaderPackage.h"
#include "RHI/Public/Shaders/ShaderPackageLayoutCatalog.h"

#include <vector>

class BindingRecordBuilder final
{
  public:
	static void Build(
	    const PassParameterLayout& layout,
	    Engine::Strings::StringTableBuilder& stringTable,
	    std::vector<CookedShaderBindingRecord>& outBindingRecords);
};
