#include "PCH.h"

#include "Backend/ShaderBackendPool.h"

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderBackendFactory.h"

IShaderBackend* ShaderBackendPool::ResolveAndAcquire(
    const std::filesystem::path& sourcePath,
    ShaderTarget target,
    std::string_view requestedName,
    std::string& outResolvedBackendName,
    std::string& outErrorMessage)
{
	outResolvedBackendName = ResolveShaderBackendName(sourcePath, target, requestedName, outErrorMessage);
	if (outResolvedBackendName.empty())
	{
		return nullptr;
	}

	return Acquire(outResolvedBackendName, target, outErrorMessage);
}

IShaderBackend* ShaderBackendPool::Find(std::string_view backendName) const noexcept
{
	const auto it = m_backends.find(std::string(backendName));
	return it != m_backends.end() ? it->second.get() : nullptr;
}

IShaderBackend* ShaderBackendPool::Acquire(std::string_view backendName, ShaderTarget target, std::string& outErrorMessage)
{
	const auto existing = m_backends.find(std::string(backendName));
	if (existing != m_backends.end())
	{
		return existing->second.get();
	}

	std::unique_ptr<IShaderBackend> backend = CreateShaderBackend(backendName, outErrorMessage);
	if (!backend)
	{
		return nullptr;
	}

	const std::string resolvedName(backend->GetBackendName());
	if (!backend->GetCapabilities().SupportsTarget(target))
	{
		outErrorMessage = std::string{"Shader backend '"} + resolvedName + "' does not support target '" +
			GetShaderTargetName(target) + "'";
		return nullptr;
	}

	IShaderBackend* backendPtr = backend.get();
	m_backends.emplace(resolvedName, std::move(backend));
	outErrorMessage.clear();
	return backendPtr;
}