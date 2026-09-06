#pragma once

#include "ShaderContractCatalog.h"

#include <cstdint>
#include <string>
#include <string_view>

enum class ShaderContractSelectionKind : std::uint8_t
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
