#include "PCH.h"

#include "Verification/ShaderParameterStructCookVerifier.h"

#include "Cooking/ShaderCookDiagnostics.h"
#include "Core/Public/Json/JsonWriter.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructVerifier.h"

#include <format>

bool ShaderParameterStructCookVerifier::Verify(
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

	const ShaderParameterStructVerificationResult verificationResult =
	    ShaderParameterStructVerifier::Verify(
	        *node.parameterStructDescriptor,
	        compiledStage.reflection);
	if (debugArtifacts != nullptr)
	{
		debugArtifacts->ParameterMatchReportJson = verificationResult.BuildJsonReport();
	}
	if (!verificationResult.succeeded)
	{
		outErrorMessage = std::format(
		    "SC2001 {} parameter-struct '{}' verification failed: {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, compiledStage.backendName, node.compileOptions.Target),
		    node.parameterStructDescriptor->Name,
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
