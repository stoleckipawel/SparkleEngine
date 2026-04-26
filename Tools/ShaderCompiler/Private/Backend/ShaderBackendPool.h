#pragma once

#include "Backend/IShaderBackend.h"
#include "Backend/ShaderTarget.h"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

class ShaderBackendPool final
{
  public:
	IShaderBackend* ResolveAndAcquire(
	    const std::filesystem::path& sourcePath,
	    ShaderTarget target,
	    std::string_view requestedName,
	    std::string& outResolvedBackendName,
	    std::string& outErrorMessage);

	IShaderBackend* Find(std::string_view backendName) const noexcept;

  private:
	IShaderBackend* Acquire(std::string_view backendName, ShaderTarget target, std::string& outErrorMessage);

	std::unordered_map<std::string, std::unique_ptr<IShaderBackend>> m_backends;
};