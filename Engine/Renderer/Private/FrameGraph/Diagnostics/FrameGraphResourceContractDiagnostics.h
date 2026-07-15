#pragma once

#include "FrameGraph/FrameGraphPassKind.h"
#include "FrameGraph/PassResourceDeclaration.h"
#include "Renderer/Public/ShaderParameters/PassParameterSet.h"

#include <string_view>
#include <vector>

namespace FrameGraphResourceContractDiagnostics
{
	bool ValidatePassDeclarations(
	    std::string_view passName,
	    EFrameGraphPassKind passKind,
	    const std::vector<PassResourceDeclaration>& declarations) noexcept;

	bool ValidatePassParameterBinding(
	    std::string_view passName,
	    const PassParameterDesc& parameter,
	    const PassParameterBinding& binding) noexcept;
}
