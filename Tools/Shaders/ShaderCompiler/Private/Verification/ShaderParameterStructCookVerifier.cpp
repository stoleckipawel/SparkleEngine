#include "PCH.h"

#include "Verification/ShaderParameterStructCookVerifier.h"

#include "Cooking/ShaderCookDiagnostics.h"
#include "Core/Public/Diagnostics/Error.h"
#include "Core/Public/Json/JsonWriter.h"
#include "ShaderDebugArtifactSet.h"
#include "Verification/ShaderParameterStructVerifier.h"

#include <format>

void ShaderParameterStructCookVerifier::Verify(
    const CookNode& node,
    const CookedStageBuild& compiledStage,
    ShaderDebugArtifactSet* debugArtifacts)
{
	if (!node.parameterStructDescriptor.has_value())
	{
		WriteSkippedReport(debugArtifacts, "no parameter-struct descriptor declared for this shader stage");
		return;
	}
	if (node.package->packageKind == CookedShaderPackageKind::RayTracingLibrary)
	{
		WriteSkippedReport(debugArtifacts, "ray-tracing library packages do not use pass parameter-struct validation");
		return;
	}

	const ShaderParameterStructVerificationResult verificationResult =
	    ShaderParameterStructVerifier::Verify(
	        *node.parameterStructDescriptor,
	        compiledStage.reflection);
	if (debugArtifacts != nullptr)
	{
		debugArtifacts->ParameterMatchReportJson = verificationResult.BuildJsonReport();
	}
	if (!verificationResult.mismatches.empty())
	{
		throw Diagnostics::Error(std::format(
		    "SC2001 {} parameter-struct '{}' verification failed: {}",
		    ShaderCookDiagnostics::FormatNodeContext(node, compiledStage.backendName, node.compileOptions.Target),
		    node.parameterStructDescriptor->Name,
		    verificationResult.mismatches.front()));
	}
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
