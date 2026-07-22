#pragma once

#include "SceneData/Input/Validation/RenderFrameMetadataValidator.h"

class RenderWorld;

class RenderInputFrameValidator final
{
  public:
	bool Validate(
	    const RenderWorld& world,
	    const RenderInputFrame& input,
	    bool& historyResetRequired,
	    std::string& diagnostic) const;
	void Commit(const RenderFrameMetadata& metadata) noexcept { m_metadata.Commit(metadata); }

  private:
	RenderFrameMetadataValidator m_metadata;
};
