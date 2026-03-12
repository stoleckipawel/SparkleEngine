// ============================================================================
// ShaderCompileResult.h
// ----------------------------------------------------------------------------
// Encapsulates the output of a shader compilation attempt.
//
#pragma once

#include <string>
#include <vector>
#include <cstdint>

// ============================================================================
// ShaderBytecode (Non-owning View)
// ============================================================================

/// Non-owning view into shader bytecode. Matches D3D12_SHADER_BYTECODE layout.
struct ShaderBytecode
{
	const void* Data = nullptr;
	size_t Size = 0;

	bool IsValid() const noexcept { return Data != nullptr && Size > 0; }
	explicit operator bool() const noexcept { return IsValid(); }
};

// Encapsulates the output of a shader compilation attempt.
// Owns the compiled bytecode and any diagnostic messages.
class ShaderCompileResult
{
  public:
	ShaderCompileResult() = default;
	~ShaderCompileResult() = default;

	ShaderCompileResult(ShaderCompileResult&&) noexcept = default;
	ShaderCompileResult& operator=(ShaderCompileResult&&) noexcept = default;

	ShaderCompileResult(const ShaderCompileResult&) = delete;
	ShaderCompileResult& operator=(const ShaderCompileResult&) = delete;

	// True if compilation succeeded and bytecode is available.
	bool IsSuccess() const noexcept { return m_success; }
	explicit operator bool() const noexcept { return m_success; }

	// Returns a non-owning view of the bytecode for PSO creation.
	ShaderBytecode GetBytecode() const noexcept { return {m_bytecode.data(), m_bytecode.size()}; }

	// Error or warning messages from the compiler.
	const std::string& GetErrorMessage() const noexcept { return m_errorMessage; }
	bool HasErrors() const noexcept { return !m_errorMessage.empty(); }

	// Factory methods for creating results.
	static ShaderCompileResult Success(std::vector<uint8_t>&& bytecode)
	{
		ShaderCompileResult result;
		result.m_success = true;
		result.m_bytecode = std::move(bytecode);
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
	std::vector<uint8_t> m_bytecode;
	std::string m_errorMessage;
};
