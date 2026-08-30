#include "PCH.h"

#include "Backend/BuiltinBackends.h"

#include <algorithm>
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

	static const std::vector<ShaderBackendRegistration>& RegistrationSnapshot()
	{
		static const std::vector<ShaderBackendRegistration> registrations = []
		{
			std::vector<ShaderBackendRegistration> snapshot = MutableRegistrations();
			std::ranges::sort(
			    snapshot,
			    [](const ShaderBackendRegistration& left, const ShaderBackendRegistration& right)
			    { return left.Descriptor.Name < right.Descriptor.Name; });
			return snapshot;
		}();
		return registrations;
	}
};

void RegisterBuiltinShaderBackend(ShaderBackendRegistration registration)
{
	ShaderBackendRegistrationStore::MutableRegistrations().push_back(registration);
}

std::span<const ShaderBackendRegistration> GetBuiltinShaderBackendRegistrations() noexcept
{
	return ShaderBackendRegistrationStore::RegistrationSnapshot();
}
