#include "PCH.h"
#include "SceneData/Input/Validation/RenderInputFrameValidator.h"

#include "SceneData/Input/Validation/RenderFrameDynamicDataValidator.h"

bool RenderInputFrameValidator::Validate(
    const RenderWorld& world,
    const RenderInputFrame& input,
    bool& historyResetRequired,
    std::string& diagnostic) const
{
	historyResetRequired = false;
	if (!m_metadata.Validate(input, historyResetRequired, diagnostic) ||
	    !RenderFrameDynamicDataValidator::Validate(world, input, diagnostic))
		return false;
	diagnostic.clear();
	return true;
}
