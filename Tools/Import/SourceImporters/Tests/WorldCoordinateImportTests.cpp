#include "Fbx/FbxAnimationImporter.h"
#include "Fbx/FbxCameraImporter.h"
#include "Fbx/FbxLightImporter.h"
#include "Gltf/GltfCoordinateConverter.h"
#include "Gltf/GltfMeshInstanceAppender.h"
#include "SourceSceneImporter.h"

#include "Core/Public/Math/WorldCoordinateSystem.h"

#include <DirectXMath.h>
#include <cgltf.h>

#include <cmath>
#include <cstdint>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace WorldCoordinateImportTests
{
	constexpr float kEpsilon = 1.0e-4f;

	void Require(bool condition, std::string_view message)
	{
		if (!condition)
		{
			throw std::runtime_error(std::string(message));
		}
	}

	bool Near(float lhs, float rhs, float epsilon = kEpsilon) noexcept
	{
		return std::abs(lhs - rhs) <= epsilon;
	}

	bool MatrixNear(DirectX::FXMMATRIX lhs, DirectX::CXMMATRIX rhs, float epsilon = kEpsilon) noexcept
	{
		DirectX::XMFLOAT4X4 lhsValues;
		DirectX::XMFLOAT4X4 rhsValues;
		DirectX::XMStoreFloat4x4(&lhsValues, lhs);
		DirectX::XMStoreFloat4x4(&rhsValues, rhs);
		const float* lhsElements = &lhsValues._11;
		const float* rhsElements = &rhsValues._11;
		for (std::uint32_t element = 0; element < 16u; ++element)
		{
			if (!Near(lhsElements[element], rhsElements[element], epsilon))
			{
				return false;
			}
		}
		return true;
	}

	bool MatrixIsIdentity(const DirectX::XMFLOAT4X4& value, float epsilon = kEpsilon) noexcept
	{
		return MatrixNear(DirectX::XMLoadFloat4x4(&value), DirectX::XMMatrixIdentity(), epsilon);
	}

	bool MatrixIsIdentity(DirectX::FXMMATRIX value, float epsilon = kEpsilon) noexcept
	{
		return MatrixNear(value, DirectX::XMMatrixIdentity(), epsilon);
	}

	void CanonicalBasisAndSemanticConversions()
	{
		Require(WorldCoordinates::kMetersPerWorldUnit == 1.0f, "One engine world unit is not one metre.");
		Require(
		    WorldCoordinates::kRightX == 1.0f && WorldCoordinates::kRightY == 0.0f && WorldCoordinates::kRightZ == 0.0f,
		    "Canonical right is not +X.");
		Require(
		    WorldCoordinates::kUpX == 0.0f && WorldCoordinates::kUpY == 1.0f && WorldCoordinates::kUpZ == 0.0f,
		    "Canonical up is not +Y.");
		Require(
		    WorldCoordinates::kForwardX == 0.0f && WorldCoordinates::kForwardY == 0.0f && WorldCoordinates::kForwardZ == 1.0f,
		    "Canonical forward is not +Z.");

		const DirectX::XMFLOAT3 sourceRight{-1.0f, 0.0f, 0.0f};
		const DirectX::XMFLOAT3 sourceUp{0.0f, 1.0f, 0.0f};
		const DirectX::XMFLOAT3 sourceForward{0.0f, 0.0f, 1.0f};
		const DirectX::XMFLOAT3 right = GltfCoordinateConverter::ConvertDirection(sourceRight);
		const DirectX::XMFLOAT3 up = GltfCoordinateConverter::ConvertNormal(sourceUp);
		const DirectX::XMFLOAT3 forward = GltfCoordinateConverter::ConvertTranslation(sourceForward);
		Require(right.x == 1.0f && right.y == 0.0f && right.z == 0.0f, "glTF semantic right did not map to engine +X.");
		Require(up.x == 0.0f && up.y == 1.0f && up.z == 0.0f, "glTF semantic up did not map to engine +Y.");
		Require(forward.x == 0.0f && forward.y == 0.0f && forward.z == 1.0f, "glTF semantic forward did not map to engine +Z.");

		const DirectX::XMFLOAT4 tangent = GltfCoordinateConverter::ConvertTangentFrame({0.25f, 0.5f, 0.75f, -1.0f});
		Require(
		    tangent.x == -0.25f && tangent.y == 0.5f && tangent.z == 0.75f && tangent.w == 1.0f,
		    "glTF tangent direction and handedness were not reflected together.");

		std::vector<std::uint32_t> indices{0u, 1u, 2u, 3u, 4u, 5u};
		GltfCoordinateConverter::ConvertTriangleWinding(indices);
		Require(indices == std::vector<std::uint32_t>({0u, 2u, 1u, 3u, 5u, 4u}), "glTF reflection did not reverse every triangle.");
	}

	void MatrixAndQuaternionConversionsAreEquivalent()
	{
		const DirectX::XMVECTOR sourceRotation = DirectX::XMQuaternionRotationRollPitchYaw(0.37f, -0.81f, 0.23f);
		DirectX::XMFLOAT4 sourceQuaternion;
		DirectX::XMStoreFloat4(&sourceQuaternion, sourceRotation);
		const DirectX::XMFLOAT4 convertedQuaternion = GltfCoordinateConverter::ConvertRotation(sourceQuaternion);
		Require(
		    MatrixNear(
		        GltfCoordinateConverter::ConvertMatrix(DirectX::XMMatrixRotationQuaternion(sourceRotation)),
		        DirectX::XMMatrixRotationQuaternion(DirectX::XMLoadFloat4(&convertedQuaternion))),
		    "glTF quaternion conversion is not equivalent to the matrix basis conversion.");

		const DirectX::XMMATRIX sourceMatrix = DirectX::XMMatrixScaling(1.5f, 0.75f, 2.0f)
		    * DirectX::XMMatrixRotationQuaternion(sourceRotation) * DirectX::XMMatrixTranslation(3.0f, -2.0f, 5.0f);
		const DirectX::XMFLOAT3 sourcePoint{1.0f, 4.0f, -3.0f};
		DirectX::XMFLOAT3 transformedSource;
		DirectX::XMStoreFloat3(&transformedSource, DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&sourcePoint), sourceMatrix));
		const DirectX::XMFLOAT3 expectedPoint = GltfCoordinateConverter::ConvertPosition(transformedSource);
		const DirectX::XMFLOAT3 convertedPoint = GltfCoordinateConverter::ConvertPosition(sourcePoint);
		DirectX::XMFLOAT3 actualPoint;
		DirectX::XMStoreFloat3(
		    &actualPoint,
		    DirectX::XMVector3TransformCoord(DirectX::XMLoadFloat3(&convertedPoint), GltfCoordinateConverter::ConvertMatrix(sourceMatrix)));
		Require(
		    Near(actualPoint.x, expectedPoint.x) && Near(actualPoint.y, expectedPoint.y) && Near(actualPoint.z, expectedPoint.z),
		    "glTF point and matrix conversion do not commute.");
	}

	void HierarchyUsesLocalThenParentWorld()
	{
		cgltf_node parent{};
		parent.has_translation = 1;
		parent.translation[0] = 2.0f;
		parent.translation[1] = -1.0f;
		parent.translation[2] = 3.0f;
		parent.has_rotation = 1;
		parent.rotation[1] = std::sin(DirectX::XM_PIDIV4);
		parent.rotation[3] = std::cos(DirectX::XM_PIDIV4);

		cgltf_node child{};
		child.parent = &parent;
		child.has_translation = 1;
		child.translation[0] = 4.0f;
		child.translation[1] = 2.0f;
		child.translation[2] = -1.0f;

		const DirectX::XMMATRIX parentLocal = GltfCoordinateConverter::ComputeNodeLocalTransform(&parent);
		const DirectX::XMMATRIX childLocal = GltfCoordinateConverter::ComputeNodeLocalTransform(&child);
		const DirectX::XMMATRIX actualWorld = GltfCoordinateConverter::ComputeNodeWorldTransform(&child);
		Require(MatrixNear(actualWorld, childLocal * parentLocal), "glTF hierarchy does not use row-vector Local * ParentWorld order.");
		Require(!MatrixNear(actualWorld, parentLocal * childLocal), "Hierarchy fixture does not distinguish reversed composition order.");
	}

	void CameraAndLightForwardIsAdaptedOnce()
	{
		const DirectX::XMMATRIX canonicalWorld = GltfCoordinateConverter::ConvertCameraOrLightWorldTransform(DirectX::XMMatrixIdentity());
		const DirectX::XMFLOAT3 direction = GltfCoordinateConverter::TransformDirection(canonicalWorld, {0.0f, 0.0f, 1.0f});
		Require(
		    Near(direction.x, 0.0f) && Near(direction.y, 0.0f) && Near(direction.z, -1.0f),
		    "An identity glTF camera/light node did not preserve its authored local -Z direction.");
	}

	void GpuInstancesComposeLocalThenNodeWorld()
	{
		float sourceTranslation[3] = {1.0f, 0.0f, 0.0f};
		cgltf_buffer buffer{};
		buffer.data = sourceTranslation;
		buffer.size = sizeof(sourceTranslation);
		cgltf_buffer_view bufferView{};
		bufferView.buffer = &buffer;
		bufferView.size = sizeof(sourceTranslation);
		cgltf_accessor accessor{};
		accessor.buffer_view = &bufferView;
		accessor.count = 1u;
		accessor.component_type = cgltf_component_type_r_32f;
		accessor.type = cgltf_type_vec3;

		GltfMeshGpuInstancingTransforms transforms;
		transforms.translations = &accessor;
		transforms.instanceCount = 1u;
		const DirectX::XMMATRIX nodeWorld = DirectX::XMMatrixRotationY(DirectX::XM_PIDIV2) * DirectX::XMMatrixTranslation(2.0f, 3.0f, 4.0f);
		SourceImportOutput output;
		GltfMeshInstanceAppender::AppendMeshGpuInstancingGroup(
		    output,
		    transforms,
		    0u,
		    0u,
		    nodeWorld,
		    kInvalidImportedSkeletonIndex,
		    0u,
		    "instance-test");
		Require(output.scene.meshInstances.size() == 1u, "GPU instance fixture did not produce one instance.");

		const DirectX::XMMATRIX authored = GltfMeshInstancingImporter::BuildMeshGpuInstancingTransform(transforms, 0u);
		const DirectX::XMMATRIX actual = DirectX::XMLoadFloat4x4(&output.scene.meshInstances[0].worldTransform);
		Require(MatrixNear(actual, authored * nodeWorld), "GPU instance does not use InstanceLocal * NodeWorld order.");
		Require(!MatrixNear(actual, nodeWorld * authored), "GPU instance fixture does not distinguish reversed composition order.");
	}

	void FbxCameraAndLightDistancesUseSourceUnitsOnce()
	{
		auto scene = std::make_unique<aiScene>();
		scene->mRootNode = new aiNode();
		scene->mRootNode->mName.Set("Root");
		scene->mRootNode->mNumChildren = 2u;
		scene->mRootNode->mChildren = new aiNode*[2u];

		auto* cameraNode = new aiNode();
		cameraNode->mName.Set("Camera");
		cameraNode->mParent = scene->mRootNode;
		scene->mRootNode->mChildren[0] = cameraNode;
		scene->mNumCameras = 1u;
		scene->mCameras = new aiCamera*[1u];
		scene->mCameras[0] = new aiCamera();
		aiCamera& sourceCamera = *scene->mCameras[0];
		sourceCamera.mName.Set("Camera");
		sourceCamera.mPosition = {100.0f, 0.0f, 300.0f};
		// Match the representation emitted by Assimp's MakeLeftHanded camera pass
		// for an authored local +Z viewing direction.
		sourceCamera.mLookAt = {200.0f, 0.0f, 599.0f};
		sourceCamera.mUp = {0.0f, 1.0f, 0.0f};
		sourceCamera.mAspect = 2.0f;
		sourceCamera.mHorizontalFOV = 0.5f;
		sourceCamera.mClipPlaneNear = 25.0f;
		sourceCamera.mClipPlaneFar = 2500.0f;

		auto* lightNode = new aiNode();
		lightNode->mName.Set("Light");
		lightNode->mParent = scene->mRootNode;
		scene->mRootNode->mChildren[1] = lightNode;
		scene->mNumLights = 1u;
		scene->mLights = new aiLight*[1u];
		scene->mLights[0] = new aiLight();
		aiLight& sourceLight = *scene->mLights[0];
		sourceLight.mName.Set("Light");
		sourceLight.mType = aiLightSource_POINT;
		sourceLight.mPosition = {100.0f, 200.0f, -300.0f};
		sourceLight.mColorDiffuse = {4.0f, 2.0f, 1.0f};
		sourceLight.mAttenuationConstant = 1.0f;
		sourceLight.mAttenuationLinear = 0.02f;
		sourceLight.mAttenuationQuadratic = 0.0003f;
		sourceLight.mSize = {50.0f, 25.0f};

		constexpr float sourceMetersPerUnit = 0.01f;
		SourceImportOutput output;
		FbxCameraImporter::ImportCameras(*scene, sourceMetersPerUnit, output);
		FbxLightImporter::ImportLights(*scene, sourceMetersPerUnit, output);
		Require(output.scene.cameras.size() == 1u && output.scene.lights.size() == 1u, "FBX fixture did not import one camera and light.");

		const ImportedCamera& camera = output.scene.cameras.front();
		Require(Near(camera.nearZ, 0.25f) && Near(camera.farZ, 25.0f), "FBX camera clip distances were not converted to metres once.");
		Require(
		    Near(camera.worldTransform._41, 1.0f) && Near(camera.worldTransform._43, -3.0f),
		    "FBX camera local offset was not reflected and converted to metres once.");
		DirectX::XMFLOAT3 cameraForward;
		DirectX::XMStoreFloat3(
		    &cameraForward,
		    DirectX::XMVector3TransformNormal(
		        DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
		        DirectX::XMLoadFloat4x4(&camera.worldTransform)));
		Require(
		    Near(cameraForward.x, 0.0f) && Near(cameraForward.y, 0.0f) && Near(cameraForward.z, -1.0f),
		    "FBX camera forward did not preserve the authored direction after handedness conversion.");

		const ImportedLight& light = output.scene.lights.front();
		Require(
		    Near(light.worldTransform._41, 1.0f) && Near(light.worldTransform._42, 2.0f) && Near(light.worldTransform._43, 3.0f),
		    "FBX light position was not reflected and converted to metres once.");
		Require(Near(light.width, 0.5f) && Near(light.height, 0.25f), "FBX light dimensions were not converted to metres once.");
		Require(
		    Near(light.distanceAttenuationCoefficients.x, 1.0f) && Near(light.distanceAttenuationCoefficients.y, 2.0f)
		        && Near(light.distanceAttenuationCoefficients.z, 3.0f),
		    "FBX attenuation coefficients were not converted to the metre distance domain.");
	}

	void FbxAnimationInputsAreFiniteAndRotationContinuous()
	{
		auto scene = std::make_unique<aiScene>();
		scene->mRootNode = new aiNode();
		scene->mRootNode->mName.Set("Joint");
		scene->mNumAnimations = 1u;
		scene->mAnimations = new aiAnimation*[1u];
		scene->mAnimations[0] = new aiAnimation();
		aiAnimation& animation = *scene->mAnimations[0];
		animation.mName.Set("Animation");
		animation.mDuration = 1.0;
		animation.mTicksPerSecond = 1.0;
		animation.mNumChannels = 1u;
		animation.mChannels = new aiNodeAnim*[1u];
		animation.mChannels[0] = new aiNodeAnim();
		aiNodeAnim& channel = *animation.mChannels[0];
		channel.mNodeName.Set("Joint");
		channel.mPreState = aiAnimBehaviour_DEFAULT;
		channel.mPostState = aiAnimBehaviour_DEFAULT;
		channel.mNumPositionKeys = 1u;
		channel.mPositionKeys = new aiVectorKey[1u];
		channel.mPositionKeys[0].mTime = 0.0;
		channel.mPositionKeys[0].mValue = {1.0f, 2.0f, 3.0f};
		channel.mNumRotationKeys = 2u;
		channel.mRotationKeys = new aiQuatKey[2u];
		channel.mRotationKeys[0].mTime = 0.0;
		channel.mRotationKeys[0].mValue = {1.0f, 0.0f, 0.0f, 0.0f};
		channel.mRotationKeys[1].mTime = 1.0;
		channel.mRotationKeys[1].mValue = {-1.0f, 0.0f, 0.0f, 0.0f};

		SourceImportOutput output;
		ImportedSkeleton skeleton;
		skeleton.joints.push_back(ImportedJoint{.name = "Joint", .sourceNodeIndex = 0u});
		output.scene.skeletons.push_back(std::move(skeleton));
		FbxAnimationImporter::ImportAnimations(*scene, output);
		Require(output.scene.animations.size() == 1u, "FBX animation fixture did not import one clip.");
		const ImportedAnimationSampler& rotationSampler = output.scene.animations.front().samplers[1u];
		Require(rotationSampler.keyframes.size() == 2u, "FBX animation fixture did not import two rotation keys.");
		Require(
		    DirectX::XMVectorGetX(
		        DirectX::XMVector4Dot(
		            DirectX::XMLoadFloat4(&rotationSampler.keyframes[0].value),
		            DirectX::XMLoadFloat4(&rotationSampler.keyframes[1].value)))
		        >= 0.0f,
		    "FBX rotation keys were not made sign-continuous.");

		channel.mPositionKeys[0].mValue.x = (std::numeric_limits<float>::quiet_NaN)();
		SourceImportOutput invalidOutput;
		ImportedSkeleton invalidSkeleton;
		invalidSkeleton.joints.push_back(ImportedJoint{.name = "Joint", .sourceNodeIndex = 0u});
		invalidOutput.scene.skeletons.push_back(std::move(invalidSkeleton));
		bool rejected = false;
		try
		{
			FbxAnimationImporter::ImportAnimations(*scene, invalidOutput);
		}
		catch (const std::exception&)
		{
			rejected = true;
		}
		Require(rejected, "FBX animation accepted a non-finite vector key.");
	}

	void CesiumManSatisfiesTheCanonicalSkinContract(const std::filesystem::path& repositoryRoot)
	{
		const std::filesystem::path sourcePath = repositoryRoot / "Projects/Showcase/Assets/Meshes/CesiumMan/CesiumMan.gltf";
		const SourceImportOutput imported = SourceSceneImporter::Import(sourcePath);
		Require(imported.HasCanonicalCoordinates(), "CesiumMan import did not publish the current coordinate-contract version.");
		Require(Near(imported.provenance.sourceMetersPerUnit, 1.0f), "glTF source units were not recorded as metres.");
		Require(!imported.scene.skeletons.empty(), "CesiumMan did not import a skeleton.");
		Require(!imported.scene.animations.empty(), "CesiumMan did not import animation.");

		bool foundSkinnedInstance = false;
		bool foundNonIdentitySkinPlacement = false;
		for (const ImportedMeshInstance& instance : imported.scene.meshInstances)
		{
			if (instance.HasSkeletonBinding())
			{
				foundSkinnedInstance = true;
				foundNonIdentitySkinPlacement = foundNonIdentitySkinPlacement || !MatrixIsIdentity(instance.worldTransform);
			}
		}
		Require(foundSkinnedInstance, "CesiumMan did not publish a skinned mesh instance.");
		Require(
		    foundNonIdentitySkinPlacement,
		    "CesiumMan non-joint Z_UP/Armature ancestry was not retained as explicit object/world placement.");

		for (const ImportedSkeleton& skeleton : imported.scene.skeletons)
		{
			for (const ImportedJoint& joint : skeleton.joints)
			{
				DirectX::XMMATRIX reconstructed =
				    DirectX::XMLoadFloat4x4(&joint.bindLocalTransform) * DirectX::XMLoadFloat4x4(&joint.parentSpaceTransform);
				if (joint.parentJointIndex < skeleton.joints.size())
				{
					reconstructed *= DirectX::XMLoadFloat4x4(&skeleton.joints[joint.parentJointIndex].bindModelTransform);
				}
				Require(
				    MatrixNear(reconstructed, DirectX::XMLoadFloat4x4(&joint.bindModelTransform), 2.0e-3f),
				    "CesiumMan joint does not satisfy BindLocal * ParentSpace * ParentBindModel == BindModel.");
				const DirectX::XMMATRIX bindSkinning =
				    DirectX::XMLoadFloat4x4(&joint.inverseBindMatrix) * DirectX::XMLoadFloat4x4(&joint.bindModelTransform);
				if (!MatrixIsIdentity(bindSkinning, 3.0e-3f))
				{
					DirectX::XMFLOAT4X4 value;
					DirectX::XMStoreFloat4x4(&value, bindSkinning);
					throw std::runtime_error(
					    std::format(
					        "CesiumMan joint '{}' bind skinning result is [{:.3f} {:.3f} {:.3f} {:.3f}; {:.3f} {:.3f} {:.3f} {:.3f}; "
					        "{:.3f} {:.3f} {:.3f} {:.3f}; {:.3f} {:.3f} {:.3f} {:.3f}].",
					        joint.name,
					        value._11,
					        value._12,
					        value._13,
					        value._14,
					        value._21,
					        value._22,
					        value._23,
					        value._24,
					        value._31,
					        value._32,
					        value._33,
					        value._34,
					        value._41,
					        value._42,
					        value._43,
					        value._44));
				}
			}
		}

		for (const ImportedAnimationClip& clip : imported.scene.animations)
		{
			for (const ImportedAnimationChannel& channel : clip.channels)
			{
				if (channel.targetPath != ImportedAnimationTargetPath::Rotation || channel.samplerIndex >= clip.samplers.size())
				{
					continue;
				}
				const ImportedAnimationSampler& sampler = clip.samplers[channel.samplerIndex];
				DirectX::XMVECTOR previous = DirectX::XMQuaternionIdentity();
				bool hasPrevious = false;
				for (const ImportedAnimationKeyframe& keyframe : sampler.keyframes)
				{
					const DirectX::XMVECTOR rotation = DirectX::XMLoadFloat4(&keyframe.value);
					Require(
					    Near(DirectX::XMVectorGetX(DirectX::XMVector4Length(rotation)), 1.0f, 2.0e-4f),
					    "Animation quaternion is not normalized.");
					Require(
					    !hasPrevious || DirectX::XMVectorGetX(DirectX::XMVector4Dot(previous, rotation)) >= -kEpsilon,
					    "Animation quaternion track is not sign-continuous.");
					previous = rotation;
					hasPrevious = true;
				}
			}
		}
	}

	using TestFunction = void (*)();

	int Run(std::string_view name, TestFunction test)
	{
		try
		{
			test();
			std::cout << "[PASS] " << name << '\n';
			return 0;
		}
		catch (const std::exception& error)
		{
			std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
			return 1;
		}
	}
}

int main(int argumentCount, char** arguments)
{
	if (argumentCount != 2)
	{
		std::cerr << "Usage: WorldCoordinateImportTests <repository-root>\n";
		return 2;
	}

	int failureCount = 0;
	failureCount += WorldCoordinateImportTests::Run(
	    "canonical basis and semantic conversions",
	    WorldCoordinateImportTests::CanonicalBasisAndSemanticConversions);
	failureCount += WorldCoordinateImportTests::Run(
	    "matrix and quaternion equivalence",
	    WorldCoordinateImportTests::MatrixAndQuaternionConversionsAreEquivalent);
	failureCount +=
	    WorldCoordinateImportTests::Run("row-vector hierarchy order", WorldCoordinateImportTests::HierarchyUsesLocalThenParentWorld);
	failureCount += WorldCoordinateImportTests::Run(
	    "camera and light forward adaptation",
	    WorldCoordinateImportTests::CameraAndLightForwardIsAdaptedOnce);
	failureCount += WorldCoordinateImportTests::Run(
	    "GPU instance composition order",
	    WorldCoordinateImportTests::GpuInstancesComposeLocalThenNodeWorld);
	failureCount += WorldCoordinateImportTests::Run(
	    "FBX camera and light units",
	    WorldCoordinateImportTests::FbxCameraAndLightDistancesUseSourceUnitsOnce);
	failureCount += WorldCoordinateImportTests::Run(
	    "FBX animation finite values and rotation continuity",
	    WorldCoordinateImportTests::FbxAnimationInputsAreFiniteAndRotationContinuous);
	try
	{
		WorldCoordinateImportTests::CesiumManSatisfiesTheCanonicalSkinContract(std::filesystem::path(arguments[1]));
		std::cout << "[PASS] CesiumMan canonical skin contract\n";
	}
	catch (const std::exception& error)
	{
		std::cerr << "[FAIL] CesiumMan canonical skin contract: " << error.what() << '\n';
		++failureCount;
	}
	return failureCount == 0 ? 0 : 1;
}
