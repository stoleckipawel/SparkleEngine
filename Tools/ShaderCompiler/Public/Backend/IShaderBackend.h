#pragma once

#include "Backend/ShaderBackendCapabilities.h"
#include "ShaderCompileOptions.h"
#include "ShaderCompileResult.h"

#include <cstdint>
#include <string_view>

// Single seam between the cooker and concrete shader backends.
// Orchestration code talks to IShaderBackend only.
class IShaderBackend
{
  public:
	virtual ~IShaderBackend() = default;

	virtual ShaderBackendCapabilities GetCapabilities() const = 0;

	// Stable backend identity for cooked binaries and cache keys.
	// Different backends must never collide on the same artifact slot.
	virtual std::string_view GetBackendName() const = 0;
	virtual std::uint64_t GetBackendVersion() const = 0;

	virtual ShaderCompileResult Compile(const ShaderCompileOptions& options) = 0;
};
