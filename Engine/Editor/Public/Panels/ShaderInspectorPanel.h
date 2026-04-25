#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

class ShaderInspectorPanel final
{
  public:
	void SetOpen(bool open) noexcept { m_isOpen = open; }
	bool IsOpen() const noexcept { return m_isOpen; }
	void BuildUI(bool disableInteraction);

  private:
	struct ArtifactBundle final
	{
		std::string Label;
		std::filesystem::path Directory;
	};

	struct ArtifactTexts final
	{
		std::string Source;
		std::string Reflection;
		std::string Disassembly;
		std::string ParameterMatch;
		std::string CompileRequest;
	};

	static std::filesystem::path GetArtifactSearchRoot();
	static std::string BuildBundleLabel(const std::filesystem::path& directory);
	static std::string ReadTextFileOrMessage(const std::filesystem::path& path);
	static void DrawTextArtifact(const char* childId, const std::string& text) noexcept;

	void RefreshBundles();
	void SelectBundle(std::size_t index);
	void LoadSelectedBundleTexts();

	std::vector<ArtifactBundle> m_bundles;
	ArtifactTexts m_texts;
	std::size_t m_selectedIndex = 0;
	bool m_hasSelection = false;
	bool m_hasScanned = false;
	bool m_isOpen = false;
};