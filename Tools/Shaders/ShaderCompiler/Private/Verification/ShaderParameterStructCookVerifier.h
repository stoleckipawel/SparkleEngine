#pragma once

#include "Cooking/CookedStageBuild.h"
#include "Cooking/CookNode.h"

#include <string>
#include <string_view>

struct ShaderDebugArtifactSet;

class ShaderParameterStructCookVerifier final
{
  public:
	ShaderParameterStructCookVerifier() = delete;

	static void Verify(
	    const CookNode& node,
	    const CookedStageBuild& compiledStage,
	    ShaderDebugArtifactSet* debugArtifacts);

  private:
	static void WriteSkippedReport(ShaderDebugArtifactSet* debugArtifacts, std::string_view reason);
};
