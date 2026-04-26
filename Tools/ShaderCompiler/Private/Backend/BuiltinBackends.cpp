#include "PCH.h"

#include "Backend/BuiltinBackends.h"

#include <vector>

class ShaderBackendRegistrationStore final
{
  public:
	ShaderBackendRegistrationStore() = delete;

	static std::vector<ShaderBackendRegistration>& MutableRegistrations()
	{
		static std::vector<ShaderBackendRegistration> registrations;
		return registrations;
	}
};

void RegisterBuiltinShaderBackend(ShaderBackendRegistration registration)
{
	ShaderBackendRegistrationStore::MutableRegistrations().push_back(registration);
}

std::span<const ShaderBackendRegistration> GetBuiltinShaderBackendRegistrations() noexcept
{
	return ShaderBackendRegistrationStore::MutableRegistrations();
}