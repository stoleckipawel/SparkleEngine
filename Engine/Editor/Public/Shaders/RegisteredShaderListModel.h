#pragma once

#include <string_view>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

struct RegisteredShaderRow final
{
	std::string ShaderId;
	std::uint64_t ShaderTypeId = 0;
	std::string SourcePath;
	std::string EntryPoint;
	std::string Stage;
	std::size_t ParameterCount = 0;
	std::uint64_t RuntimeGeneration = 0;
	bool ArtifactAvailable = false;
	std::filesystem::path ArtifactDirectory;
};

class RegisteredShaderListModel final
{
public:
	using GenerationProvider = std::function<std::uint64_t()>;

	void SetGenerationProvider(GenerationProvider provider);
	void Refresh();

	const std::vector<RegisteredShaderRow>& GetRows() const noexcept { return m_rows; }

private:
	static std::filesystem::path FindDebugArtifactDirectoryFor(std::string_view shaderId);

	GenerationProvider m_generationProvider;
	std::vector<RegisteredShaderRow> m_rows;
};
