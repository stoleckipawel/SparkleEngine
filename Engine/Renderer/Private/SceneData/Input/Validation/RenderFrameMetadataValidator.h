#pragma once

#include "Rendering/RenderInputFrame.h"

#include <cstdint>
#include <string>

class RenderFrameMetadataValidator final
{
  public:
	bool Validate(
	    const RenderInputFrame& input,
	    bool& historyResetRequired,
	    std::string& diagnostic) const;
	void Commit(const RenderFrameMetadata& metadata) noexcept;

  private:
	std::uint64_t m_frameId = 0;
	std::uint64_t m_frameGeneration = 0;
	std::uint64_t m_providerGeneration = 0;
	bool m_hasAcceptedFrame = false;
};
