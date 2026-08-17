#include "Assets/Cooked/LoadedAnimationAsset.h"
#include "Assets/Cooked/LoadedSceneManifest.h"
#include "Assets/Cooked/LoadedSkeletonAsset.h"
#include "Assets/Loaders/AnimationAssetLoader.h"
#include "Assets/Loaders/MeshAssetLoader.h"
#include "Assets/Loaders/SceneManifestValidator.h"
#include "Assets/Loaders/SkeletonAssetLoader.h"

#include "GameFramework/Public/Assets/Cooked/CookedAnimationAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedMeshAsset.h"
#include "GameFramework/Public/Assets/Cooked/CookedSceneManifest.h"
#include "GameFramework/Public/Assets/Cooked/CookedSkeletonAsset.h"

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace CookedCoordinateContractTests
{
	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	template <typename Value> std::vector<std::uint8_t> BytesOf(const Value& value)
	{
		static_assert(std::is_trivially_copyable_v<Value>);
		std::vector<std::uint8_t> bytes(sizeof(Value));
		std::memcpy(bytes.data(), &value, sizeof(Value));
		return bytes;
	}

	template <typename Operation> void RequireRejected(Operation&& operation, std::string_view message)
	{
		try
		{
			operation();
		}
		catch (const std::exception&)
		{
			return;
		}
		throw std::runtime_error(std::string(message));
	}

	void EverySpatialArtifactRejectsAnotherCoordinateContract()
	{
		const std::uint32_t staleVersion = WorldCoordinates::kCoordinateContractVersion + 1u;

		Assets::CookedMeshAssetHeader meshHeader;
		meshHeader.coordinateContractVersion = staleVersion;
		const std::vector<std::uint8_t> meshBytes = BytesOf(meshHeader);
		RequireRejected(
		    [&] { (void) Assets::MeshAssetLoader{}.Decode(std::filesystem::path("stale.mesh"), meshBytes); },
		    "Mesh loader accepted another coordinate-contract version.");

		Assets::CookedSkeletonAssetHeader skeletonHeader;
		skeletonHeader.coordinateContractVersion = staleVersion;
		const std::vector<std::uint8_t> skeletonBytes = BytesOf(skeletonHeader);
		RequireRejected(
		    [&] { (void) Assets::SkeletonAssetLoader{}.Decode(std::filesystem::path("stale.skeleton"), skeletonBytes); },
		    "Skeleton loader accepted another coordinate-contract version.");

		Assets::CookedAnimationAssetHeader animationHeader;
		animationHeader.coordinateContractVersion = staleVersion;
		const std::vector<std::uint8_t> animationBytes = BytesOf(animationHeader);
		RequireRejected(
		    [&] { (void) Assets::AnimationAssetLoader{}.Decode(std::filesystem::path("stale.animation"), animationBytes); },
		    "Animation loader accepted another coordinate-contract version.");

		Assets::LoadedSceneManifest manifest;
		manifest.header.coordinateContractVersion = staleVersion;
		RequireRejected(
		    [&] { Assets::SceneManifestValidator::ValidateHeader(manifest); },
		    "Scene manifest validator accepted another coordinate-contract version.");
	}
}

int main()
{
	try
	{
		CookedCoordinateContractTests::EverySpatialArtifactRejectsAnotherCoordinateContract();
		std::cout << "[PASS] cooked coordinate-contract rejection\n";
		return 0;
	}
	catch (const std::exception& error)
	{
		std::cerr << "[FAIL] cooked coordinate-contract rejection: " << error.what() << '\n';
		return 1;
	}
}
