#pragma once

#include "Backend/IShaderBackend.h"

#include <memory>

// Backend selection seam for tool orchestration.
// Callers still need to inspect IsValid() and GetCapabilities().
std::unique_ptr<IShaderBackend> CreateDefaultShaderBackend();
