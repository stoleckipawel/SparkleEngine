#pragma once

#include "SceneData/RenderWorld.h"

class RenderWorldDeltaValidator final
{
  public:
	static RenderWorldApplyStatus Validate(
	    const RenderWorld& world,
	    const RenderWorldDelta& delta,
	    std::string& diagnostic);

  private:
	static RenderWorldApplyStatus ValidateSequence(
	    const RenderWorld& world,
	    const RenderWorldDelta& delta,
	    std::string& diagnostic);
	static bool ValidateOperations(
	    const RenderWorld& world,
	    const RenderWorldDelta& delta,
	    std::string& diagnostic);
	static bool ValidateResources(
	    const RenderWorld& world,
	    const RenderWorldDelta& delta,
	    std::string& diagnostic);
};
