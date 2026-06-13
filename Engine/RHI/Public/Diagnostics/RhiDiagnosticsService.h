#pragma once

#include "../Diagnostics/RhiDiagnostics.h"
#include "../RHIAPI.h"

class SPARKLE_RHI_API RhiDiagnosticsService
{
  public:
	virtual ~RhiDiagnosticsService() noexcept = default;

	virtual RenderDiagnostics& GetDiagnostics() noexcept = 0;
	virtual const RenderDiagnostics& GetDiagnostics() const noexcept = 0;
};
