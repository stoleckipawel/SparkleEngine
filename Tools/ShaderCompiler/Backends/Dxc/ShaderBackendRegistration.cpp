#include "PCH.h"

#include "Dxc/ShaderBackendRegistration.h"

#include "Dxc/DxcShaderBackend.h"

static std::unique_ptr<IShaderBackend> CreateBackendInstance()
{
	return std::make_unique<DxcShaderBackend>();
}

ShaderBackendRegistration GetDxcBackendRegistration() noexcept
{
	return ShaderBackendRegistration{
	    .name = "dxc",
	    .create = &CreateBackendInstance,
	};
}

struct DxcBackendRegistrar final
{
	DxcBackendRegistrar()
	{
		RegisterBuiltinShaderBackend(GetDxcBackendRegistration());
	}
};

static DxcBackendRegistrar g_dxcBackendRegistrar;