# Indirect Lighting Artifact Postmortem

Status: resolved and user-verified

Date: 2026-07-12

Scene: Showcase / Sponza

## Symptoms

- Saturated pink, violet, green, or blue lines appeared in Lit, Indirect Diffuse, and Indirect Specular.
- The artifacts were strongest at object silhouettes and changed as the camera moved.
- The sky could become uniformly magenta.
- Primary GBuffer and direct-lighting debug views were clean.
- Raster and ray-traced GBuffer modes both reproduced the indirect-lighting symptom.

## Root cause

The environment texture descriptor had an invalid lifetime.

Frame-graph execution constructs render-pass helper objects locally while recording commands. `SkyPass` and the ray-traced indirect passes owned an `EnvironmentMapPassBinding`; that binding allocated its SRV descriptor table during `Execute` and released it when the local pass object was destroyed. The GPU could consume the command list after the descriptor slot had been returned to the allocator and reused for another resource.

Sky and indirect lighting therefore sampled a changing, unrelated descriptor. This explains the animated colors, the full-screen magenta sky, and the matching colors reflected at object edges.

The fix gives the long-lived `TextureManager` a general shader-resource binding cache. Pass orchestration resolves the environment as an ordinary texture and requests its persistent binding through the same texture-level API available to other consumers.

Relevant code:

- `Engine/Renderer/Private/Textures/TextureManager.h`
- `Engine/Renderer/Private/Textures/TextureManager.cpp`
- `Engine/Renderer/Private/Passes/Deferred/SkyPass.cpp`
- `Engine/Renderer/Private/Passes/Bindings/RayTracedSurfaceLightingPassBinding.h`

## Supporting evidence

- The cooked 4096-by-2048 HDR environment contained finite, plausible radiance and no sampled magenta texels.
- A fixed-UV sky sample was magenta.
- `Texture2D.Load`, which bypasses the sampler, remained magenta.
- Forcing the environment resolver to the 16-by-16 checker texture still produced magenta.
- `SkyTexture.GetDimensions` observed neither the checker nor environment dimensions before the lifetime fix.
- The user confirmed the artifact was gone after running the editor with the persistent descriptor fix and a normal Sky shader package.

## Related accepted fixes

- Uploaded D3D12 textures transition from `COPY_DEST` to `ALL_SHADER_RESOURCE`, because the same assets are read by pixel, compute, and ray-query shaders.
- Raster motion vectors use raster-space `SV_Position`, explicitly remove current jitter, and add the current-to-previous jitter delta during temporal reprojection. The user separately confirmed the camera-rotation teleporting defect is fixed.
- GGX reflection sampling uses visible-normal distribution sampling and its matching PDF.
- Sky radiance sampling is independent of screen-space view reconstruction. Passes that reconstruct positions or directions bind temporal view data explicitly through their shared parameter contract.
- Shader math does not silently replace non-finite radiance, throughput, motion, or reservoir values. Failures remain visible at their originating stage.

## Rejected investigation changes

The final changelist does not retain the following experiments:

- environment-disable or checker-texture overrides;
- quadrant, band, fixed-UV, or dimension visualization shaders;
- arbitrary indirect contribution clamps;
- roughness encoded into packed normal length;
- grazing-angle or edge rejection;
- global HDR output clipping;
- exposure, ray-bias, reconstruction-provider, or GBuffer default overrides;
- horizontal-cross atlas inference for the current equirectangular environment.

## Validation

- Build the `ShowcaseEditor` and `ShaderCompiler` DevelopmentEditor targets.
- Cook affected packages from `Projects/Showcase` without selecting a single target; the package must contain both DXIL SM 6.6 and SPIR-V 1.6.
- Check Lit, Indirect Diffuse, and Indirect Specular while moving and rotating the camera.
- Repeat Lit with raster and ray-traced GBuffer modes.
- Confirm real indirect illumination remains present; a black indirect buffer is not an acceptable fix.
