#include "PCH.h"

#include "FrameGraph/Diagnostics/FrameGraphResourceContractDiagnostics.h"

#include "FrameGraph/FrameGraphPassFlags.h"
#include "FrameGraph/ResourceUsage.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <cassert>
#include <string>

namespace
{
	static const auto g_frameGraphContractLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

	bool ReportValidationFailure(std::string_view passName, std::string_view message) noexcept
	{
		std::string logMessage = "FrameGraph resource contract validation failed for pass '";
		logMessage.append(passName.begin(), passName.end());
		logMessage += "': ";
		logMessage.append(message.begin(), message.end());
		Diagnostics::Fail(g_frameGraphContractLogger, __FILE__, __LINE__, logMessage);
	}
}

bool FrameGraphResourceContractDiagnostics::ValidatePassDeclarations(
    std::string_view passName,
    EFrameGraphPassFlags flags,
    const std::vector<PassResourceDeclaration>& declarations) noexcept
{
	assert(HasExactlyOnePassKind(flags));

	for (const PassResourceDeclaration& declaration : declarations)
	{
		if (ReadsFromUsage(declaration.usage) || WritesToUsage(declaration.usage))
		{
			continue;
		}

		std::string message = "parameter '";
		message += declaration.label.empty() ? "<unlabeled>" : declaration.label;
		message += "' uses unsupported resource usage ";
		message += ResourceUsageToString(declaration.usage);
		message += ".";
		return ReportValidationFailure(passName, message);
	}

	return true;
}

bool FrameGraphResourceContractDiagnostics::ValidatePassParameterBinding(
    std::string_view passName,
    const PassParameterDesc& parameter,
    const PassParameterBinding& binding) noexcept
{
	if (parameter.Kind != ShaderParameterSemanticKind::AccelerationStructure)
	{
		return true;
	}

	const PassParameterAccelerationStructureBindingData* accelerationStructureData = binding.AsAccelerationStructureData();
	if (accelerationStructureData == nullptr)
	{
		return ReportValidationFailure(passName, "acceleration-structure parameter binding type did not match the reflected layout.");
	}

	if (!accelerationStructureData->Handle.IsValid())
	{
		std::string message = "acceleration-structure parameter '";
		message += parameter.Name;
		message += "' must be bound through a FrameGraph acceleration-structure handle so setup, compile, and diagnostics can track it.";
		return ReportValidationFailure(passName, message);
	}

	return true;
}
