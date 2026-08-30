#pragma once

#include "ShaderContractCatalog.h"

#include <string>
#include <string_view>

enum class ShaderContractSelectionKind
{
	All,
	ShaderId,
};

class ShaderContractCatalogBuilder final
{
public:
	ShaderContractCatalogBuilder() = delete;

	static ShaderContractCatalog Build(ShaderContractSelectionKind selectionKind, std::string_view requestedId);
};
