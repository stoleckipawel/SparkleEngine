#pragma once

#include <cstddef>
#include <string>
#include "World/EntityId.h"

class EditorTransactionHistory;
struct WorldMeshReadData;

class SceneMeshInspector final
{
public:
	static void Build(const WorldMeshReadData&, EditorTransactionHistory&, std::uint64_t, const std::string&) noexcept;

private:
	static void BuildTransformCategory(const std::string&, const WorldMeshReadData&, EditorTransactionHistory&, std::uint64_t) noexcept;
	static void BuildStaticMeshCategory(const std::string&, const WorldMeshReadData&) noexcept;
	static void BuildAdvancedParametersCategory(
	    const std::string&,
	    const WorldMeshReadData&,
	    EditorTransactionHistory&,
	    std::uint64_t) noexcept;
	static void BuildMaterialsCategory(const std::string&, const WorldMeshReadData&) noexcept;

	static constexpr float kPositionSliderMin = -500.0f;
	static constexpr float kPositionSliderMax = 500.0f;
	static constexpr float kScaleSliderMin = 0.001f;
	static constexpr float kScaleSliderMax = 100.0f;
};
