#pragma once

#include "ShaderContractCatalog.h"

#include <string>
#include <vector>

class ShaderContractValidator final
{
  public:
	ShaderContractValidator() = delete;

	static std::vector<ShaderContractVerificationFailure> Validate(const ShaderContractCatalog& catalog);
	static std::string FormatFailure(const ShaderContractVerificationFailure& failure);
};
