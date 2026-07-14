# Indirect-Lighting Artifact Investigation and Postmortem

Status: resolved, user-verified, and cleanup-complete

Investigation closed: 2026-07-14

Scene: Showcase / Sponza

## Result

The animated pink, violet, green, and blue artifacts were caused by invalid descriptor/view lifetime for the sky texture. The descriptor used by visible Sky and indirect-lighting passes could be returned to the allocator while the recorded command list still referenced it. Later allocations reused the slot, so the GPU sampled an unrelated resource.

The final solution does not keep the temporary persistent-descriptor cache that first confirmed the diagnosis. Sky and ordinary uploaded buffers now use the generalized frame-graph resource path; the graph creates the views required by declared pass semantics, and the RHI descriptor service retires native views only after the corresponding frame-in-flight slot is GPU-safe.

## Observed symptoms

- Saturated pink, violet, green, or blue lines appeared in Lit, Indirect Diffuse, and Indirect Specular.
- The artifacts were strongest at silhouettes and grazing reflections.
- Their color and position changed while the camera moved.
- The visible sky could become uniformly magenta.
- Direct Lighting and primary GBuffer debug views were clean.
- Raster and ray-traced GBuffer modes both reproduced the indirect symptom.
- The same failure appeared in indirect diffuse and indirect specular, pointing to shared input or resource infrastructure rather than two independent BRDF failures.

## How the evidence narrowed the search

### 1. Primary visibility was not the common cause

The clean GBuffer and Direct Lighting views ruled against a corrupt primary material buffer, normal decode, or light buffer as the sole cause. Reproduction with both raster and ray-traced GBuffer paths ruled against either primary-visibility implementation.

### 2. The shared indirect input was the sky

Both indirect lobes sample sky radiance on ray misses. The discovery that the visible sky itself could become fully magenta moved the investigation upstream of diffuse/specular evaluation. Edge concentration was consistent with sky visibility and reflection weighting: silhouettes and grazing directions expose miss radiance strongly, so a broken sky sample appeared as narrow colored contours before it became obvious as a full-screen sky failure.

### 3. The HDR asset and UV math were not sufficient explanations

The cooked 4096-by-2048 HDR texture contained finite, plausible radiance and no sampled magenta texels. Fixed UVs, `Texture2D.Load`, and a checker replacement still observed the wrong result. `GetDimensions` did not observe the expected texture dimensions. Those tests showed that the shader was not reliably seeing the intended resource, regardless of sampling coordinates or filtering.

### 4. Resource identity changed after command recording

The old pass-local sky binding allocated a descriptor while recording a pass and released it when the local helper was destroyed. GPU execution is asynchronous: destruction of the CPU helper did not mean the GPU had consumed the descriptor. Descriptor reuse then changed which resource the already-recorded command referenced. This directly explains the animated colors, resource-dimension mismatch, and camera-dependent appearance.

## Root cause

The lifetime model conflated three events:

1. a CPU pass helper finished recording commands;
2. CPU ownership of a descriptor wrapper ended;
3. the GPU finished using the native descriptor and view.

Only the third event permits native reclamation. The old implementation reclaimed at the second event.

The defect was architectural rather than Sky-specific. A descriptor allocator cannot infer GPU completion from a C++ wrapper destructor, and a pass object should not own persistent resource views. A Sky-only longer-lived wrapper would have hidden the same missing generalized lifetime contract.

## Final implementation

### Scene and renderer identity

- `SceneSkyDesc` owns level-authoring values and an asset reference.
- `SceneSkySnapshot` is the value-copy boundary between mutable game-framework state and renderer consumption.
- `RenderSceneDataBuilder` resolves the asset reference once into `RenderSkyData`.
- `RenderSkyData` stores a renderer `Texture*` plus color, intensity, and enabled state; it stores no descriptor or graph handle.

### Frame-graph registration

- frame assembly reserves one persistent external Sky texture handle;
- frame orchestration binds the current renderer texture before graph setup;
- every visible-sky and indirect pass declares its own `ShaderTexture2D` use through `CreateSRV`;
- the graph derives resource access and transitions from those declarations;
- resource or metadata changes release generated views and cause the graph to materialize the new required view.

### RHI lifetime

- `RhiDescriptorService` owns resource-view records;
- `ReleaseResourceView` logically releases the public handle but retains the native record in a per-frame queue;
- common `RenderDeviceServices::BeginFrame` asks the descriptor service to reclaim the current slot only after the backend has waited for that slot;
- D3D12 recycles descriptor allocations at that point;
- Vulkan releases registered descriptor entries and destroys owned `VkImageView` objects at that point;
- Vulkan registered-descriptor indices are returned to their free list instead of growing indefinitely.

The frame graph no longer carries a second retirement queue. GPU lifetime policy lives with the RHI object that actually owns the native view.

### Explicit pass binding

Resource declarations remain visible in each pass. The cleanup removed feature-specific binding objects, shared parameter-list macros, and `IndirectLightingOutputPassBinding`-style facades. Common code is retained only where it represents one coherent operation, such as Sky radiance evaluation or conversion of `RenderSkyData` to the exact GPU constant-buffer layout.

## Related correctness fixes

These were found during the broader investigation and solve distinct defects; they are not presented as alternate explanations for the descriptor-lifetime artifact.

### Raster motion vectors

Raster motion uses raster-space `SV_Position`, removes the current jitter explicitly, and applies the current-to-previous jitter delta when locating temporal history. This fixed the separate camera-rotation shifting/teleporting symptom. The user verified that defect independently before the sky artifact was closed.

### D3D12 texture state

Uploaded textures transition from `COPY_DEST` to `D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE`, because the same texture can be read by pixel, compute, and ray-query work. Restricting the resource to pixel-shader state was invalid for the indirect compute/ray paths. This state correction is necessary even though descriptor reuse was the cause of the animated false-color sampling.

### Cross-RHI validation findings

Running the same cooked packages under Vulkan validation exposed parity defects that D3D12 did not diagnose at startup. They were fixed in generalized backend/compiler contracts:

- a TLAS initially built for later refit now submits the same allow-update mode used for prebuild sizing on both APIs;
- persistent history textures are physically created in the undefined state and let the graph establish their first read/write state, instead of claiming shader-read state before any transition occurred;
- the Vulkan descriptor allocator derives required descriptor counts from the compiled binding layout, so a 4096-entry material table is not allocated from a fixed 1024-sampled-image page;
- the shader cooker assigns one logical package binding to the same SPIR-V set/binding in every stage, removing the raster GBuffer's vertex/pixel constant-buffer mismatch;
- SPIR-V uses DXC's documented formatless storage-image mode, with the corresponding Vulkan device features required and enabled, so `RWTexture2D<float4>` can legally address graph textures such as `R16G16B16A16_FLOAT`;
- shared resource-view validation runs before either backend creates a descriptor;
- Vulkan upload/state scopes cover graphics and compute shader reads, matching D3D12's all-shader-resource state.
- sampled-image array non-uniform indexing is exposed as one neutral RHI capability; Renderer no longer disables the material table by checking for Vulkan directly;
- native texture state translation is performed by each RHI backend, so the Streamline Renderer adapter no longer embeds D3D12 resource-state bit values;
- graph participation was removed from the public RHI shader layout and is now carried by Renderer typed-parameter metadata.

These are parity fixes, not Sky-specific workarounds. The final Vulkan validation run contained no descriptor, image-layout, typed-storage-image, acceleration-structure, feature-enable, or device-loss diagnostics.

### GGX visible-normal sampling

Specular sampling uses the GGX visible-normal distribution and a matching reflection-direction PDF. The sampler and PDF were changed together; mixing a full-NDF sampler with a VNDF PDF would bias indirect specular. This is a PBR correctness improvement, not an artifact clamp.

`SampleVisibleGGXHalfVector` is the isotropic HLSL translation of Heitz's published routine: stretch the view by alpha, sample the projected disk, reproject to the hemisphere, and unstretch the sampled normal. The visible-normal density is

```text
p(m) = D(m) * G1(v) * |v.m| / |n.v|
```

Reflection contributes the Jacobian `1 / (4 * |v.m|)`, so the matching direction density implemented by Sparkle is `D(m) * G1(v) / (4 * |n.v|)`. This is why the old full-NDF density could not remain after changing the sampler. See Heitz's [Sampling the GGX Distribution of Visible Normals](https://www.jcgt.org/published/0007/04/01/paper.pdf), whose Listing 1 matches the implemented transform. NVIDIA Falcor's [8.0 release notes](https://github.com/NVIDIAGameWorks/Falcor/releases/tag/8.0) document production VNDF sampling and its later spherical-cap update; AMD's [GPUOpen publications catalog](https://gpuopen.com/learn/publications/) documents its continuing Smith-GGX VNDF research. No output sanitization, arbitrary radiance clipping, or edge rejection remains in the solution.

## Rejected theories and removed diagnostics

The following were useful probes but are not in the final implementation:

- sky-disable and checker overrides;
- fixed-UV, quadrant, band, dimension, and texture-identity visualizations;
- switching from filtered sampling to `Texture2D.Load`;
- treating the equirectangular source as a horizontal-cross atlas;
- roughness encoded into packed normal length;
- grazing-angle or object-edge rejection;
- arbitrary diffuse, specular, throughput, or HDR contribution clamps;
- global `isfinite` replacement colors;
- exposure, ray-bias, reconstruction-provider, or GBuffer default changes;
- assuming the issue was unique to either raster or ray-traced GBuffer;
- retaining a Sky-specific persistent binding cache after the general graph path existed.

Failures are intentionally not hidden by broad shader sanitization. Invalid data should remain visible at the earliest shader that produces it, with validation at ownership and upload boundaries.

## Cleanup and deduplication record

The completed cleanup ensures that the investigation did not leave a second rendering architecture:

- one name, Sky, is used for Sparkle's visible and sampled sky resource;
- old environment-specific binding helpers are deleted;
- the temporary texture-manager binding cache is deleted;
- ordinary scene buffers no longer prebuild their own SRVs;
- uploaded frame buffers share one move-only RAII owner instead of four release implementations;
- graph construction and runtime rebinding APIs are separated;
- unused general import and builder pass-through APIs are deleted;
- texture format metadata has one authority, `PixelFormat`, rather than an enum plus stored duplicate string;
- constant-buffer requirements are explicit static assertions instead of a parameterized macro;
- snapshots remain because they provide a distinct lifetime and future threading boundary.
- the public Renderer facade no longer exposes its complete RHI device or command-submission service to Application.

## External architecture support

- NVIDIA NVRHI documents automatic resource-state tracking and deferred safe destruction through command-list-held references, and creates native views from binding descriptions inside the abstraction. See the [NVRHI programming guide](https://github.com/NVIDIA-RTX/NVRHI/blob/main/doc/ProgrammingGuide.md).
- AMD Render Pipeline Shaders declares graph resource access, derives barriers, and manages external, persistent, transient, and temporal resource lifetimes. See the [RPS resource tutorial](https://github.com/GPUOpen-LibrariesAndSDKs/RenderPipelineShaders/blob/main/docs/tutorial/rps_tutorial_p2.md).
- Epic RDG derives dependencies, barriers, and lifetimes from pass parameters and registers external resources into the graph before graph-created view use. See Epic's [Render Dependency Graph guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/render-dependency-graph-in-unreal-engine).
- NVIDIA RTXDI exercises one HLSL codebase through D3D12 and Vulkan using NVRHI and DXC SPIR-V, supporting Sparkle's package-level cross-backend contract. See the [RTXDI repository](https://github.com/NVIDIA-RTX/RTXDI).
- Microsoft's DXC SPIR-V documentation defines both Vulkan binding behavior and `-fspv-use-unknown-image-format`, which Sparkle uses instead of custom SPIR-V mutation for storage-image formats. See the [DXC SPIR-V guide](https://github.com/microsoft/DirectXShaderCompiler/blob/main/docs/SPIR-V.rst).

Sparkle retains its persistent graph rather than copying Unreal's immediate graph. The shared principle is that high-level passes declare resource use while graph/RHI owners manage native state and lifetime.

## Verification

Completed verification:

- `ShowcaseEditor` DevelopmentEditor build;
- `ShaderCompiler` DevelopmentEditor build;
- both D3D12 and Vulkan backend compilation;
- architecture-boundary check;
- cache-disabled cook of all 28 packages, producing 58 DXIL SM 6.6 / SPIR-V 1.6 stage jobs with no failures;
- D3D12 runtime smoke for at least 15 seconds without an engine error or device loss;
- Vulkan runtime smoke for at least 15 seconds with zero validation warnings or errors;
- editor startup without the former crash;
- moving and rotating the camera in Lit, Indirect Diffuse, and Indirect Specular;
- Lit checked with raster and ray-traced GBuffer modes;
- user confirmation that the colored artifact and the motion-vector teleporting symptom were gone;
- real indirect illumination remained present, ruling out a black-output workaround.

## Regression checklist

When changing Sky, descriptors, or external graph resources:

1. inspect Lit, Indirect Diffuse, and Indirect Specular while moving the camera;
2. repeat Lit with both GBuffer modes;
3. view the sky directly and in grazing reflections;
4. change Sky texture, color, intensity, and enabled state and confirm history invalidates immediately;
5. switch levels repeatedly to exercise external rebinding and descriptor retirement;
6. verify both DXIL and SPIR-V packages;
7. confirm indirect lighting remains physically present rather than clamped or cleared.
