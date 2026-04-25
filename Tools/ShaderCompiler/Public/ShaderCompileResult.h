#pragma once

#include "RHI/Public/Shaders/ShaderBytecode.h"
#include "ShaderDebugArtifactSet.h"
#include "ShaderReflection.h"

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

// Output of one shader compile invocation for the offline tool.
// ShaderBytecode stays under RHI public because runtime readers return it.
class ShaderCompileResult
{
  public:
	ShaderCompileResult() = default;
	~ShaderCompileResult() = default;

	ShaderCompileResult(ShaderCompileResult&&) noexcept = default;
	ShaderCompileResult& operator=(ShaderCompileResult&&) noexcept = default;

	ShaderCompileResult(const ShaderCompileResult&) = delete;
	ShaderCompileResult& operator=(const ShaderCompileResult&) = delete;

	bool IsSuccess() const noexcept { return m_success; }
	explicit operator bool() const noexcept { return m_success; }

	ShaderBytecode GetBytecode() const noexcept { return {m_bytecode.data(), m_bytecode.size()}; }
	const std::filesystem::path& GetDebugArtifactPath() const noexcept { return m_debugArtifactPath; }

	const std::string& GetErrorMessage() const noexcept { return m_errorMessage; }
	bool HasErrors() const noexcept { return !m_errorMessage.empty(); }

	const ShaderReflection& GetReflection() const noexcept { return m_reflection; }
	ShaderReflection&& TakeReflection() noexcept { return std::move(m_reflection); }
	void SetReflection(ShaderReflection&& reflection) noexcept { m_reflection = std::move(reflection); }
	const ShaderDebugArtifactSet& GetDebugArtifacts() const noexcept { return m_debugArtifacts; }
	ShaderDebugArtifactSet&& TakeDebugArtifacts() noexcept { return std::move(m_debugArtifacts); }
	void SetDebugArtifacts(ShaderDebugArtifactSet&& debugArtifacts) noexcept { m_debugArtifacts = std::move(debugArtifacts); }

	static ShaderCompileResult Success(std::vector<std::uint8_t>&& bytecode, std::filesystem::path debugArtifactPath = {})
	{
		ShaderCompileResult result;
		result.m_success = true;
		result.m_bytecode = std::move(bytecode);
		result.m_debugArtifactPath = std::move(debugArtifactPath);
		return result;
	}

	static ShaderCompileResult Failure(std::string&& errorMessage)
	{
		ShaderCompileResult result;
		result.m_success = false;
		result.m_errorMessage = std::move(errorMessage);
		return result;
	}

  private:
	bool m_success = false;
	std::vector<std::uint8_t> m_bytecode;
	std::filesystem::path m_debugArtifactPath;
	std::string m_errorMessage;
	ShaderReflection m_reflection;
	ShaderDebugArtifactSet m_debugArtifacts;
};
