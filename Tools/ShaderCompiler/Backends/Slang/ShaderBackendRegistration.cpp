#include "PCH.h"

#include "Slang/ShaderBackendRegistration.h"

#include "Slang/SlangShaderBackend.h"

static std::unique_ptr<IShaderBackend> CreateBackendInstance()
{
	return std::make_unique<SlangShaderBackend>();
}

ShaderBackendRegistration GetSlangBackendRegistration() noexcept
{
	return ShaderBackendRegistration{
	    .name = "slang",
	    .create = &CreateBackendInstance,
	};
}

struct SlangBackendRegistrar final
{
	SlangBackendRegistrar()
	{
		RegisterBuiltinShaderBackend(GetSlangBackendRegistration());
	}
};

static SlangBackendRegistrar g_slangBackendRegistrar;