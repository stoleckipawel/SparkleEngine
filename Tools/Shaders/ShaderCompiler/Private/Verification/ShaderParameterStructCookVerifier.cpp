#include "PCH.h"

#include "Verification/ShaderParameterStructCookVerifier.h"

#include "Cooking/ShaderCookDiagnostics.h"
#include "Cooking/ShaderPackageCooker.h"
#include "Core/Public/Json/JsonWriter.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructVerifier.h"

#include <format>

bool ShaderParameterStructCookVerifier::Verify(
    const ShaderPackageCookSettings& settings,
    const CookNode& node,
    const CookedStageBuild& compiledStage,
    ShaderDebugArtifactSet* debugArtifacts,
    std::string& outErrorMessage)
{
	if (!node.parameterStructDescriptor.has_value())
	{
		WriteSkippedReport(debugArtifacts, "no parameter-struct descriptor declared for this shader stage");
		outErrorMessage.clear();
		return true;
	}
	if (node.package->packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		WriteSkippedReport(debugArtifacts, "ray-tracing library packages do not use pass parameter-struct validation");
		outErrorMessage.clear();
		return true;
	}

	ShaderParameterStructDescriptor descriptor = *node.parameterStructDescriptor;
	if (settings.forceParameterStructMismatchForValidation)
	{
		descriptor.Fields.push_back(ShaderParameterStructFieldDescriptor{
		    .Name = "__DeliberateMissingBindingForSelfTest",
		    .LayoutName = "__DeliberateMissingBindingForSelfTest",
		    .ShaderName = "__DeliberateMissingBindingForSelfTest",
		    .Kind = CookedShaderResourceKind::ConstantBuffer,
		    .Dimension = CookedShaderResourceDimension::Buffer,
		    .SemanticKind = ShaderParameterSemanticKind::UniformData,
		    .ResourceDomain = ShaderParameterResourceDomain::Uniform,
		    .Access = ShaderParameterAccess::None,
		    .ArrayCount = 1,
		    .ValueSizeInBytes = sizeof(std::uint32_t),
		    .ValueAlignmentInBytes = alignof(std::uint32_t),
		    .Reflected = true});
	}

	const ShaderParameterStructVerificationResult verificationResult =
	    ShaderParameterStructVerifier::Verify(descriptor, compiledStage.reflection);
	if (debugArtifacts != nullptr)
	{
		debugArtifacts->ParameterMatchReportJson = verificationResult.BuildJsonReport();
	}
	if (!verificationResult.succeeded)
	{
		outErrorMessage = std::format(
		    "SC2001 {} parameter-struct '{}' verification failed: {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, compiledStage.backendName, node.compileOptions.Target),
		    descriptor.Name,
		    verificationResult.diagnostics.empty() ? "unknown mismatch" : verificationResult.diagnostics.front());
		return false;
	}

	outErrorMessage.clear();
	return true;
}

void ShaderParameterStructCookVerifier::WriteSkippedReport(ShaderDebugArtifactSet* debugArtifacts, std::string_view reason)
{
	if (debugArtifacts == nullptr)
	{
		return;
	}

	Json::ObjectWriter writer;
	writer.WriteString("status", "skipped");
	writer.WriteString("reason", reason);
	debugArtifacts->ParameterMatchReportJson = writer.Finish();
}
