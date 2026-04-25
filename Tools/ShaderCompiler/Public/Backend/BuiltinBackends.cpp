#include "PCH.h"

#include "Backend/BuiltinBackends.h"

#include <vector>

static std::vector<ShaderBackendRegistration>& MutableShaderBackendRegistrations()
{
	static std::vector<ShaderBackendRegistration> registrations;
	return registrations;
}

void RegisterBuiltinShaderBackend(ShaderBackendRegistration registration)
{
	MutableShaderBackendRegistrations().push_back(registration);
}

std::span<const ShaderBackendRegistration> GetBuiltinShaderBackendRegistrations() noexcept
{
	return MutableShaderBackendRegistrations();
}