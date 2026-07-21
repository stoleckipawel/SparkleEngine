#pragma once

#include <string>

class EditorTransactionManager;
struct WorldCameraReadData;

class SceneCameraInspector final
{
  public:
	static void Build(
	    const WorldCameraReadData& camera,
	    EditorTransactionManager& transactions,
	    std::uint64_t worldGeneration,
	    const std::string& filterText) noexcept;

  private:
	static void BuildTransformCategory(const std::string&, const WorldCameraReadData&, EditorTransactionManager&, std::uint64_t) noexcept;
	static void BuildCameraCategory(const std::string&, const WorldCameraReadData&, EditorTransactionManager&, std::uint64_t) noexcept;
	static void BuildMovementCategory(const std::string&, const WorldCameraReadData&, EditorTransactionManager&, std::uint64_t) noexcept;
	static void BuildAdvancedParametersCategory(const std::string&, const WorldCameraReadData&, EditorTransactionManager&, std::uint64_t) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;
};
