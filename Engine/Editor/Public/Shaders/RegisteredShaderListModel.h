#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

struct RegisteredShaderRow final
{
	std::string ShaderId;
	std::string PackageId;
	std::string SourcePath;
	std::string EntryPoint;
	std::string Stage;
	std::string BindingLayoutId;
	std::size_t ParameterCount = 0;
	std::size_t PermutationDimensionCount = 0;
	std::uint64_t RuntimeGeneration = 0;
	bool ArtifactAvailable = false;
	std::string LastStatus;
};

class RegisteredShaderListModel final
{
  public:
	using GenerationProvider = std::function<std::uint64_t()>;

	void SetGenerationProvider(GenerationProvider provider);
	void Refresh();
	void SetLastStatus(std::string status);

	const std::vector<RegisteredShaderRow>& GetRows() const noexcept { return m_rows; }

  private:
	static bool HasDebugArtifactsFor(std::string_view shaderId, std::string_view packageId);

	GenerationProvider m_generationProvider;
	std::vector<RegisteredShaderRow> m_rows;
	std::string m_lastStatus;
};
