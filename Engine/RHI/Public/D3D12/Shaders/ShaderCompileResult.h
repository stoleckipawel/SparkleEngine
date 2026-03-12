#pragma once

#include <string>
#include <vector>
#include <cstdint>

struct ShaderBytecode
{
	const void* Data = nullptr;
	size_t Size = 0;

	bool IsValid() const noexcept { return Data != nullptr && Size > 0; }
	explicit operator bool() const noexcept { return IsValid(); }
};

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

	const std::string& GetErrorMessage() const noexcept { return m_errorMessage; }
	bool HasErrors() const noexcept { return !m_errorMessage.empty(); }

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
