#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/CookNode.h"

#include <string>
#include <string_view>

struct ShaderDebugArtifactSet;
struct ShaderPackageCookSettings;

class ShaderParameterStructCookVerifier final
{
  public:
	ShaderParameterStructCookVerifier() = delete;

	static bool Verify(
	    const ShaderPackageCookSettings& settings,
	    const CookNode& node,
	    const CookedStageBuild& compiledStage,
	    ShaderDebugArtifactSet* debugArtifacts,
	    std::string& outErrorMessage);

  private:
	static void WriteSkippedReport(ShaderDebugArtifactSet* debugArtifacts, std::string_view reason);
};