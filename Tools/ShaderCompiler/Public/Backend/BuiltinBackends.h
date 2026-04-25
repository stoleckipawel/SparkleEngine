#pragma once

#include "Backend/IShaderBackend.h"

#include <memory>
#include <span>
#include <string_view>

struct ShaderBackendRegistration final
{
	std::string_view name;
	std::unique_ptr<IShaderBackend> (*create)();
};

void RegisterBuiltinShaderBackend(ShaderBackendRegistration registration);
std::span<const ShaderBackendRegistration> GetBuiltinShaderBackendRegistrations() noexcept;