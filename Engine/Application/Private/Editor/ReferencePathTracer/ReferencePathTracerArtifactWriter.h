#pragma once

#include "Editor/ReferencePathTracer/ReferencePathTracerArtifact.h"

#include <stop_token>

class ReferencePathTracerArtifactWriter final
{
public:
	static ReferencePathTracerArtifactWriteResult Write(
	    ReferencePathTracerArtifactWriteRequest request,
	    std::stop_token cancellationToken) noexcept;
};
