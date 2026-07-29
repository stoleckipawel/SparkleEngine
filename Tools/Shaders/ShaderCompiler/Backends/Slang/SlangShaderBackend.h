#pragma once

#include "Backend/IShaderBackend.h"

#include <slang-com-ptr.h>
#include <slang.h>

#include <string>
#include <vector>

class SlangShaderBackend final : public IShaderBackend
{
  public:
	SlangShaderBackend();
	~SlangShaderBackend() override = default;

	SlangShaderBackend(const SlangShaderBackend&) = delete;
	SlangShaderBackend& operator=(const SlangShaderBackend&) = delete;
	SlangShaderBackend(SlangShaderBackend&&) = delete;
	SlangShaderBackend& operator=(SlangShaderBackend&&) = delete;

	bool IsValid() const noexcept { return m_globalSession != nullptr; }
	static ShaderBackendCapabilities GetStaticCapabilities() noexcept;
	static std::uint64_t QueryBackendVersion();

	ShaderBackendCapabilities GetCapabilities() const override;
	std::string_view GetBackendName() const override;
	std::uint64_t GetBackendVersion() const override;
	CompiledShader Compile(const ShaderCompileOptions& options) override;

  private:
	static SlangStage MapStage(ShaderStage stage);
	static SlangCompileTarget MapTarget(ShaderTarget target);
	static std::string BlobToString(slang::IBlob* blob);
	static std::vector<std::string> BuildDebugArgumentStrings(const ShaderCompileOptions& options);
	static ShaderDebugArtifactSet CaptureDebugArtifacts(
	    const ShaderCompileOptions& options,
	    std::string_view sourceText,
	    std::string_view diagnostics);
	static std::uint64_t QueryBackendVersion(slang::IGlobalSession& globalSession);

	Slang::ComPtr<slang::IGlobalSession> m_globalSession;
	std::uint64_t m_backendVersion = 0;
};
