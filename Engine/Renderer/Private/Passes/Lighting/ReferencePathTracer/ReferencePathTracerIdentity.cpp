#include "PCH.h"

#include "Passes/Lighting/ReferencePathTracer/ReferencePathTracerIdentity.h"

#include "Core/Public/Hash/HashUtils.h"
#include "Frame/RenderFrameIdentity.h"
#include "Passes/Lighting/LightingSceneState.h"
#include "Passes/Lighting/LightingStateHash.h"
#include "Scene/Preparation/PreparedRenderScene.h"
#include "View/RenderView.h"

enum IdentityComponentIndex : std::size_t
{
	ViewComponent,
	CameraComponent,
	GeometryComponent,
	DeformationComponent,
	MaterialComponent,
	LightComponent,
	EnvironmentComponent,
	ShaderComponent,
	ExecutionComponent,
	BackendComponent,
};

static_assert(BackendComponent + 1u == ReferencePathTracerIdentity::ComponentCount);

ReferencePathTracerIdentityComponent ReferencePathTracerIdentity::FindFirstDifference(
    const ReferencePathTracerIdentity& other) const noexcept
{
	for (std::size_t componentIndex = 0; componentIndex < Components.size(); ++componentIndex)
	{
		if (Components[componentIndex] != other.Components[componentIndex])
		{
			return static_cast<ReferencePathTracerIdentityComponent>(componentIndex + 1u);
		}
	}
	return ReferencePathTracerIdentityComponent::None;
}

ReferencePathTracerIdentity BuildReferencePathTracerIdentity(
    const RenderView& view,
    const PreparedRenderScene& scene,
    const RenderFrameIdentity& frame,
    std::uint64_t sceneGeneration,
    RayTracingExecutionFrontend executionFrontend,
    ERhiBackendApi backendApi) noexcept
{
	ReferencePathTracerIdentity identity;
	std::uint64_t hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, view.viewportId);
	hash = Hash::ContinueFnv1a64Value(hash, view.selection.Value);
	identity.Components[ViewComponent] = Hash::FinalizeFnv1a64(hash);

	hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, view.renderExtent.Width);
	hash = Hash::ContinueFnv1a64Value(hash, view.renderExtent.Height);
	hash = Hash::ContinueFnv1a64Value(hash, view.camera.ProjectionKind);
	hash = LightingStateHash::AppendFloat3(hash, view.camera.Position);
	hash = LightingStateHash::AppendFloat3(hash, view.camera.Direction);
	hash = Hash::ContinueFnv1a64Value(hash, view.camera.FovYDegrees);
	hash = Hash::ContinueFnv1a64Value(hash, view.camera.AspectRatio);
	hash = Hash::ContinueFnv1a64Value(hash, view.camera.NearZ);
	hash = Hash::ContinueFnv1a64Value(hash, view.camera.FarZ);
	hash = LightingStateHash::AppendMatrix(hash, view.cameraUniform.InvViewMTX);
	hash = LightingStateHash::AppendMatrix(hash, view.cameraUniform.InvProjectionMTX);
	identity.Components[CameraComponent] = Hash::FinalizeFnv1a64(hash);

	const LightingSceneStateIdentity sceneIdentity = BuildLightingSceneStateIdentity(scene);
	hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, sceneGeneration);
	hash = Hash::ContinueFnv1a64Value(hash, scene.structuralRevision);
	hash = Hash::ContinueFnv1a64Value(hash, sceneIdentity.Geometry);
	identity.Components[GeometryComponent] = Hash::FinalizeFnv1a64(hash);
	identity.Components[DeformationComponent] = sceneIdentity.Deformation;
	identity.Components[MaterialComponent] = sceneIdentity.Materials;
	identity.Components[LightComponent] = sceneIdentity.Lights;
	identity.Components[EnvironmentComponent] = sceneIdentity.Environment;

	hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, frame.ShaderGeneration);
	identity.Components[ShaderComponent] = Hash::FinalizeFnv1a64(hash);
	hash = Hash::kFnv64OffsetBasis;
	hash = Hash::ContinueFnv1a64Value(hash, executionFrontend);
	identity.Components[ExecutionComponent] = Hash::FinalizeFnv1a64(hash);
	identity.Components[BackendComponent] = Hash::FinalizeFnv1a64(Hash::ContinueFnv1a64Value(Hash::kFnv64OffsetBasis, backendApi));
	return identity;
}
